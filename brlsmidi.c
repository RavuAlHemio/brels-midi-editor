/*

    Copyright (C) 2003-2011  Breno de Lima Sarmento

	This file is part of BRELS MIDI Editor.

    BRELS MIDI Editor is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    BRELS MIDI Editor is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with BRELS MIDI Editor.  If not, see <http://www.gnu.org/licenses/>.

	E-mail.: breno@sarmen.to
	Website: http://midi.brels.net

 */

#include <math.h>
#include <windows.h>
#include "brlsmidi.h"

typedef struct
{
	BRELS_TRACK_HEADER Header;
	DWORD dwEvents;
	DWORD dwLength;
	LPBRELS_MIDI_EVENT Events;
	DWORD dwCurrentEvent;
	BOOL Mute;
} BRELS_MIDI_TRACK, *LPBRELS_MIDI_TRACK;

typedef struct
{
	BRELS_MIDI_HEADER Header;
	LPBRELS_MIDI_TRACK Tracks;
	WORD wTracks;
	DWORD dwEvents;
	DWORD dwTempos;
	LPBRELS_TEMPO lpTempos;
	DWORD dwDevice;
	HMIDIOUT Handle;
	WORD wBeatSize;
	DWORD dwCurrentTempo;
	double TempoTime;
	double TempoTicks;
	double CurrentTicks;
	double CurrentTime;
	double TickInterval;
	double Skipped;
	BOOL Loop;
	DWORD SoloTrack;
	LARGE_INTEGER qwLastTime;
	LARGE_INTEGER qwFrequency;
	BOOL Suspended;
	DWORD dwStatus;
	QWORD Precision;
	HWND Callback;
	UINT CallbackMessage;
	BOOL Processed;
	BOOL Played;
} __attribute__ ((packed, aligned(1)))
BRELS_MIDI_SEQUENCE, *LPBRELS_MIDI_SEQUENCE;

/* Helper functions prototypes */

void BuildTempoTable(LPBRELS_MIDI_SEQUENCE lpSequence);
DWORD GetTrackSize(LPBRELS_MIDI_SEQUENCE lpSequence, WORD wTrack);
void GetTrackData(LPBRELS_MIDI_SEQUENCE lpSequence, WORD wTrack, LPBYTE* lpData);
void AdjustPosition(LPBRELS_MIDI_SEQUENCE lpSequence, WORD wTrack);
void CALLBACK PlayProc(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
void PlayCallback(LPBRELS_MIDI_SEQUENCE lpSequence, BOOL Ignore);
int Compare(void* data1, void* data2);

__declspec(dllexport) BOOL WINAPI MidiOpen(LPCSTR lpstrFile, DWORD dwDevice, LPHANDLE lphSequence)
{
	int i, j, Offset, Position, RunningStatus, Ticks, Event;
	DWORD dwOutput, Delta, nEvents;
	HANDLE Arquivo;
	LPBRELS_MIDI_SEQUENCE lpSequence;
	LPBRELS_MIDI_EVENT lpTemp;
	LARGE_INTEGER qwQuery;
	LPBYTE lpData;

	if (IsBadReadPtr(lphSequence, sizeof(BRELS_MIDI_SEQUENCE))) return FALSE;

	lpSequence = (LPBRELS_MIDI_SEQUENCE) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BRELS_MIDI_SEQUENCE));
	if (lpSequence == NULL) return FALSE;
	ZeroMemory(lpSequence, sizeof(BRELS_MIDI_SEQUENCE));

	if (dwDevice != NO_DEVICE)
	{
		if (midiOutOpen(&lpSequence->Handle, dwDevice, (DWORD_PTR) NULL, (DWORD_PTR) NULL, (DWORD) CALLBACK_NULL)!=MMSYSERR_NOERROR)
		{
			HeapFree(GetProcessHeap(), 0, lpSequence);
			return FALSE;
		}
	}
	lpSequence->dwDevice = dwDevice;

	Arquivo = CreateFile(lpstrFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (Arquivo == INVALID_HANDLE_VALUE) return FALSE;

	/* Lê o header */
	ReadFile(Arquivo, &lpSequence->Header.dwMagic  , 4, &dwOutput, NULL);
	ReadFile(Arquivo, &lpSequence->Header.dwSize   , 4, &dwOutput, NULL);
	ReadFile(Arquivo, &lpSequence->Header.wFormat  , 2, &dwOutput, NULL);
	ReadFile(Arquivo, &lpSequence->Header.wTracks  , 2, &dwOutput, NULL);
	ReadFile(Arquivo, &lpSequence->Header.wBeatSize, 2, &dwOutput, NULL);

	/* Valida o arquivo */
	if (btldw(lpSequence->Header.dwMagic)!=0x4D546864)
	{
		if (dwDevice!=NO_DEVICE) midiOutClose(lpSequence->Handle);
		HeapFree(GetProcessHeap(), 0, lpSequence);
		CloseHandle(Arquivo);
		return FALSE;
	}

	lpSequence->wBeatSize = btlw(lpSequence->Header.wBeatSize);
	lpSequence->wTracks = btlw(lpSequence->Header.wTracks);

	lpSequence->Tracks = (LPBRELS_MIDI_TRACK) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BRELS_MIDI_TRACK)*lpSequence->wTracks);


	if ((lpSequence->Tracks == NULL) && (lpSequence->wTracks>0))
	{
		if (dwDevice!=NO_DEVICE) midiOutClose(lpSequence->Handle);
		HeapFree(GetProcessHeap(), 0, lpSequence);
		CloseHandle(Arquivo);
		return FALSE;
	}

	/* Lê as trilhas */
	for (i=0; i<lpSequence->wTracks; i++)
	{
		ReadFile(Arquivo, &lpSequence->Tracks[i].Header.dwMagic , 4, &dwOutput, NULL);
		ReadFile(Arquivo, &lpSequence->Tracks[i].Header.dwLength, 4, &dwOutput, NULL);
		lpSequence->Tracks[i].dwLength = btldw(lpSequence->Tracks[i].Header.dwLength);
		lpData = (LPBYTE) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpSequence->Tracks[i].dwLength);
		if (!ReadFile(Arquivo, lpData, lpSequence->Tracks[i].dwLength, &dwOutput, NULL))
		{
			if (dwDevice!=NO_DEVICE) midiOutClose(lpSequence->Handle);
			/*if (!IsBadCodePtr((FARPROC) lpData)) */HeapFree(GetProcessHeap(), 0, lpData);
			HeapFree(GetProcessHeap(), 0, lpSequence->Tracks);
			HeapFree(GetProcessHeap(), 0, lpSequence);
			CloseHandle(Arquivo);
			return FALSE;
		}

		/* Decodifica os eventos */

		lpTemp = (LPBRELS_MIDI_EVENT) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BRELS_MIDI_EVENT)*lpSequence->Tracks[i].dwLength);
		Offset = 0;
		Ticks = 0;
		nEvents = 0;
		RunningStatus = -1;
		while (Offset < (int) lpSequence->Tracks[i].dwLength)
		{
			/* Lê o delta */
			Delta = 0;
			for (j=0; j<4; j++)
			{
				Delta = (Delta << 7) | (lpData[Offset++] & 127);
				if (lpData[Offset-1] < 128) break;
			}

			Ticks+=Delta;

			if (lpData[Offset]>0x7F)
			{
				Event = lpData[Offset++];
			}
			else Event = RunningStatus;

			lpTemp[nEvents].dwTicks = Ticks;

			if (Event==0xFF)
			{
				lpTemp[nEvents].Event = lpData[Offset++];
				lpTemp[nEvents].DataSize = lpData[Offset++];
				lpTemp[nEvents].Data.p = (LPBYTE) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpTemp[nEvents].DataSize);
				CopyMemory(lpTemp[nEvents].Data.p, &lpData[Offset], lpTemp[nEvents].DataSize);
				Offset+=lpTemp[nEvents].DataSize;
				RunningStatus = -1;
			}
			else
			if ((Event>=0x80) && (Event<=0xEF))
			{
				lpTemp[nEvents].Event = Event;
				if ((Event>=0xC0) && (Event<=0xDF))
				{
					lpTemp[nEvents].DataSize = 1;
					lpTemp[nEvents].Data.a[0] = lpData[Offset++];
				}
				else
				{
					lpTemp[nEvents].DataSize = 2;
					lpTemp[nEvents].Data.a[0] = lpData[Offset++];
					lpTemp[nEvents].Data.a[1] = lpData[Offset++];
				}
				RunningStatus = Event;
			}
			else
			{
				switch (Event)
				{
				case 0xF0:
					/*System Exclusive */
					/*Procura pelo F7 correspondente */
					Position = Offset;
					while (lpData[Offset]!=0xF7) Offset++;
					Offset++;
					lpTemp[nEvents].Event = Event;
					lpTemp[nEvents].DataSize = Offset - Position;
					lpTemp[nEvents].Data.p = (LPBYTE) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpTemp[nEvents].DataSize);
					CopyMemory(lpTemp[nEvents].Data.p, &lpData[Position], lpTemp[nEvents].DataSize);
					break;
				case 0xF1:
					/*MTC Quarter Frame Message */
					lpTemp[nEvents].Event = Event;
					lpTemp[nEvents].DataSize = 1;
					lpTemp[nEvents].Data.a[0] = lpData[Offset++];
					break;
				case 0xF2:
					/*Song Position Pointer */
					lpTemp[nEvents].Event = Event;
					lpTemp[nEvents].DataSize = 2;
					lpTemp[nEvents].Data.a[0] = lpData[Offset++];
					lpTemp[nEvents].Data.a[1] = lpData[Offset++];
					break;
				case 0xF3:
					/*Song Select */
					lpTemp[nEvents].Event = Event;
					lpTemp[nEvents].DataSize = 1;
					lpTemp[nEvents].Data.a[0] = lpData[Offset++];
					break;
				case 0xF6: /*Tune Request */
				case 0xF8: /*MIDI Clock */
				case 0xFA: /*MIDI Start */
				case 0xFB: /*MIDI Continue */
				case 0xFC: /*MIDI Stop */
				case 0xFE: /*Active sense */
				default:   /*Undefined MIDI Events */
					lpTemp[nEvents].Event = Event;
					lpTemp[nEvents].DataSize = 0;
					break;
				}
			}
			nEvents++;
		}

		lpSequence->dwEvents += nEvents;
		lpSequence->Tracks[i].dwEvents = nEvents;

		lpSequence->Tracks[i].Events = (LPBRELS_MIDI_EVENT) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BRELS_MIDI_EVENT)*nEvents);
		CopyMemory(lpSequence->Tracks[i].Events, lpTemp, nEvents * sizeof(BRELS_MIDI_EVENT));

		HeapFree(GetProcessHeap(), 0, lpData);
		HeapFree(GetProcessHeap(), 0, lpTemp);
	}

	BuildTempoTable(lpSequence);
	lpSequence->dwCurrentTempo = 500000;
	lpSequence->TempoTime = 0;
	lpSequence->TempoTicks = 0;
	lpSequence->CurrentTicks = 0 ;
	lpSequence->CurrentTime = 0;
	lpSequence->qwLastTime.QuadPart = 0;
	lpSequence->TickInterval = 500000.0l / lpSequence->wBeatSize;
	lpSequence->Loop = FALSE;
	lpSequence->SoloTrack = NO_SOLO;
	lpSequence->Precision = 1000;
	lpSequence->Processed = TRUE;
	lpSequence->Played = TRUE;
	lpSequence->dwStatus = MIDI_STOP;
	timeBeginPeriod(1);

	CloseHandle(Arquivo);

	*lphSequence = (HANDLE) lpSequence;

	QueryPerformanceFrequency(&qwQuery);
	lpSequence->qwFrequency = qwQuery;

	return TRUE;
}

__declspec(dllexport) BOOL WINAPI MidiClose(HANDLE hSequence)
{
	int i;
	MSG msg;
	LPBRELS_MIDI_SEQUENCE lpSequence;

	if (IsBadReadPtr(hSequence, sizeof(BRELS_MIDI_SEQUENCE))) return FALSE;
	lpSequence = (LPBRELS_MIDI_SEQUENCE) hSequence;

	lpSequence->dwStatus = MIDI_STOP;
	lpSequence->Processed = FALSE;
	while (!lpSequence->Played) continue;

	if (lpSequence->CallbackMessage!=0)
		while (PeekMessage(&msg, NULL, lpSequence->CallbackMessage, lpSequence->CallbackMessage, PM_REMOVE))
			continue;

	midiOutClose(lpSequence->Handle);

	for (i=0; i<lpSequence->wTracks; i++)
		MidiCleanEvents(lpSequence->Tracks[i].dwEvents, lpSequence->Tracks[i].Events);

	HeapFree(GetProcessHeap(), 0, lpSequence->Tracks);
	HeapFree(GetProcessHeap(), 0, lpSequence->lpTempos);
	HeapFree(GetProcessHeap(), 0, lpSequence);

	timeEndPeriod(1);

	return TRUE;
}

__declspec(dllexport) BOOL WINAPI MidiCreate(DWORD dwDevice, WORD wBeatSize, LPHANDLE lphSequence)
{
	LPBRELS_MIDI_SEQUENCE lpSequence;
	LARGE_INTEGER qwQuery;

	if (wBeatSize==0) return FALSE;

	lpSequence = (LPBRELS_MIDI_SEQUENCE) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BRELS_MIDI_SEQUENCE));
	if (lpSequence == NULL) return FALSE;
	ZeroMemory(lpSequence, sizeof(BRELS_MIDI_SEQUENCE));

	if (dwDevice != NO_DEVICE)
	{
		if (midiOutOpen(&lpSequence->Handle, dwDevice, (DWORD_PTR) NULL, (DWORD_PTR) NULL, (DWORD) CALLBACK_NULL)!=MMSYSERR_NOERROR)
		{
			HeapFree(GetProcessHeap(), 0, lpSequence);
			return FALSE;
		}
	}

	lpSequence->Header.dwMagic = btldw(0x4D546864);
	lpSequence->Header.dwSize = btldw(0x00000006);
	lpSequence->Header.wFormat = btlw(0x0001);
	lpSequence->Header.wTracks = btlw(0x0000);
	lpSequence->Header.wBeatSize = btlw(wBeatSize);
	lpSequence->dwDevice = dwDevice;
	lpSequence->wBeatSize = wBeatSize;
	lpSequence->dwCurrentTempo = 500000;
	lpSequence->TempoTime = 0;
	lpSequence->TempoTicks = 0;
	lpSequence->CurrentTicks = 0 ;
	lpSequence->CurrentTime = 0;
	lpSequence->qwLastTime.QuadPart = 0;
	lpSequence->TickInterval = 500000.0l / lpSequence->wBeatSize;
	lpSequence->Loop = FALSE;
	lpSequence->SoloTrack = NO_SOLO;
	lpSequence->Precision = 1000;
	lpSequence->Processed = TRUE;
	lpSequence->Played = TRUE;
	lpSequence->dwStatus = MIDI_STOP;
	timeBeginPeriod(1);

	QueryPerformanceFrequency(&qwQuery);
	lpSequence->qwFrequency = qwQuery;

	*lphSequence = (HANDLE) lpSequence;

	return TRUE;
}

__declspec(dllexport) BOOL WINAPI MidiEncode(HANDLE hSequence, LPCSTR lpstrFile, BOOL OverWrite)
{
	int i;
	LPBYTE lpData;
	DWORD dwCreateDisposition, dwOutput;
	LPBRELS_MIDI_SEQUENCE lpSequence;
	HANDLE Arquivo;

	if (IsBadReadPtr(hSequence, sizeof(BRELS_MIDI_SEQUENCE))) return FALSE;
	lpSequence = (LPBRELS_MIDI_SEQUENCE) hSequence;

	if (OverWrite)
		dwCreateDisposition = CREATE_ALWAYS;
	else
		dwCreateDisposition = CREATE_NEW;

	Arquivo = CreateFile(lpstrFile, GENERIC_WRITE, 0, NULL, dwCreateDisposition, 0, NULL);
	if (Arquivo == INVALID_HANDLE_VALUE) return FALSE;

	WriteFile(Arquivo, &lpSequence->Header.dwMagic  , 4, &dwOutput, NULL);
	WriteFile(Arquivo, &lpSequence->Header.dwSize   , 4, &dwOutput, NULL);
	WriteFile(Arquivo, &lpSequence->Header.wFormat  , 2, &dwOutput, NULL);
	WriteFile(Arquivo, &lpSequence->Header.wTracks  , 2, &dwOutput, NULL);
	WriteFile(Arquivo, &lpSequence->Header.wBeatSize, 2, &dwOutput, NULL);

	for (i=0; i<lpSequence->wTracks; i++)
	{
		lpSequence->Tracks[i].Header.dwLength = btldw(GetTrackSize(lpSequence, i));
		WriteFile(Arquivo, &lpSequence->Tracks[i].Header.dwMagic , 4, &dwOutput, NULL);
		WriteFile(Arquivo, &lpSequence->Tracks[i].Header.dwLength, 4, &dwOutput, NULL);

		GetTrackData(lpSequence, i, &lpData);
		if (!WriteFile(Arquivo, lpData, btldw(lpSequence->Tracks[i].Header.dwLength), &dwOutput, NULL))
		{
			MidiFreeBuffer(lpData);
			CloseHandle(Arquivo);
			return FALSE;
		}
		MidiFreeBuffer(lpData);
	}


	CloseHandle(Arquivo);
	return TRUE;
}


__declspec(dllexport) BOOL WINAPI MidiNext(HANDLE hSequence)
{
	int Finalizadas, Ignoradas, i;
	double Elapsed;
	LPBRELS_MIDI_EVENT Event;
	LPBRELS_MIDI_SEQUENCE lpSequence;
	LARGE_INTEGER qwQuery, qwLast;
	BOOL Played;

	if (IsBadReadPtr(hSequence, sizeof(BRELS_MIDI_SEQUENCE))) return FALSE;
	lpSequence = (LPBRELS_MIDI_SEQUENCE) hSequence;

	Played = FALSE;

	if (lpSequence->Suspended) return Played;

	Finalizadas = 0;
	Ignoradas = 0;
	Elapsed = 0;

	while ((Finalizadas+Ignoradas)!=lpSequence->wTracks)
	{

		Finalizadas = 0;
		Ignoradas = 0;

		for (i=0; i<lpSequence->wTracks; i++)
		{

			if (lpSequence->Tracks[i].dwCurrentEvent < lpSequence->Tracks[i].dwEvents)
			{
				QueryPerformanceCounter(&qwLast);
				Event = &lpSequence->Tracks[i].Events[lpSequence->Tracks[i].dwCurrentEvent];
				if (Event->dwTicks <= lpSequence->CurrentTicks)
				{
					if ((Event->Event>=0x80) && (Event->Event<=0x9F))
					{
						if ((lpSequence->SoloTrack==NO_SOLO) || (((int) lpSequence->SoloTrack)==i))
							if (!lpSequence->Tracks[i].Mute)
								midiOutShortMsg(lpSequence->Handle, Event->Event | (Event->Data.a[0] << 8) | (Event->Data.a[1] << 16));
					}
					else
						if ((Event->Event>=0xA0) && (Event->Event<=0xEF))
						{
							midiOutShortMsg(lpSequence->Handle, Event->Event | (Event->Data.a[0] << 8) | (Event->Data.a[1] << 16));
						}
						else
						{
							if (Event->Event==0x51)
							{
								lpSequence->TempoTicks = Event->dwTicks;
								lpSequence->TempoTime = lpSequence->CurrentTime;
								lpSequence->dwCurrentTempo = (Event->Data.p[0] << 16) | (Event->Data.p[1] << 8) | (Event->Data.p[2]);
							}
						}
					lpSequence->Tracks[i].dwCurrentEvent++;
					Played = TRUE;

					/* Plays all scheduled events of the current track before going to the next */
					if (lpSequence->Tracks[i].dwCurrentEvent < lpSequence->Tracks[i].dwEvents)
						if (lpSequence->Tracks[i].Events[lpSequence->Tracks[i].dwCurrentEvent].dwTicks<= lpSequence->CurrentTicks)
							i--;
				}
				else Ignoradas++;

				QueryPerformanceCounter(&qwQuery);
				Elapsed = Elapsed + ((qwQuery.QuadPart - qwLast.QuadPart) / (double) lpSequence->qwFrequency.QuadPart);
			}
			else Finalizadas++;
		}
	}

	lpSequence->TickInterval = lpSequence->dwCurrentTempo * 1.0l / lpSequence->wBeatSize;
	QueryPerformanceCounter(&qwQuery);
	if (lpSequence->qwLastTime.QuadPart==0) lpSequence->qwLastTime = qwQuery;
	Elapsed = Elapsed + (qwQuery.QuadPart - lpSequence->qwLastTime.QuadPart) * 1000000.0l / lpSequence->qwFrequency.QuadPart;
	lpSequence->CurrentTicks = lpSequence->CurrentTicks + Elapsed / lpSequence->TickInterval;
	lpSequence->CurrentTime  = lpSequence->CurrentTime + Elapsed;
	lpSequence->qwLastTime = qwQuery;
	lpSequence->Skipped = ((double) lpSequence->Precision) - Elapsed;
	if (lpSequence->Skipped<0) lpSequence->Skipped = 0;

	if (Finalizadas==lpSequence->wTracks)
	{
		Played = TRUE;
		if (lpSequence->Loop)
		{
			MidiReset(hSequence);
			MsgWaitForMultipleObjects(0, NULL, FALSE, 5, 0);
		}
		else
		{
			lpSequence->dwStatus = MIDI_STOP;
			MidiReset(hSequence);
		}
	}

	return Played;
}

__declspec(dllexport) BOOL WINAPI MidiSuspend(HANDLE hSequence, BOOL Synchronize)
{
	LPBRELS_MIDI_SEQUENCE lpSequence;

	if (IsBadReadPtr(hSequence, sizeof(BRELS_MIDI_SEQUENCE))) return FALSE;
	lpSequence = (LPBRELS_MIDI_SEQUENCE) hSequence;

	lpSequence->Suspended = TRUE;
	while (!lpSequence->Played) continue;

	if (Synchronize)
		lpSequence->qwLastTime.QuadPart = 0;

	return TRUE;
}

__declspec(dllexport) BOOL WINAPI MidiResume(HANDLE hSequence, BOOL Modified)
{
	int i;
	LPBRELS_MIDI_SEQUENCE lpSequence;

	if (IsBadReadPtr(hSequence, sizeof(BRELS_MIDI_SEQUENCE))) return FALSE;
	lpSequence = (LPBRELS_MIDI_SEQUENCE) hSequence;

	if (Modified)
		for (i=0; i<lpSequence->wTracks; i++)
			AdjustPosition(lpSequence, i);

	lpSequence->Suspended = FALSE;

	return TRUE;
}

__declspec(dllexport) BOOL WINAPI MidiSilence(HANDLE hSequence)
{
	int i;
	LPBRELS_MIDI_SEQUENCE lpSequence;

	if (IsBadReadPtr(hSequence, sizeof(BRELS_MIDI_SEQUENCE))) return FALSE;
	lpSequence = (LPBRELS_MIDI_SEQUENCE) hSequence;

	for (i=0; i<16; i++)  midiOutShortMsg(lpSequence->Handle, 0xB0 | i | (120 << 8));

	return TRUE;
}

__declspec(dllexport) BOOL WINAPI MidiReset(HANDLE hSequence)
{
	int i;
	LPBRELS_MIDI_SEQUENCE lpSequence;

	if (IsBadReadPtr(hSequence, sizeof(BRELS_MIDI_SEQUENCE))) return FALSE;
	lpSequence = (LPBRELS_MIDI_SEQUENCE) hSequence;

	lpSequence->dwCurrentTempo = 500000;
	lpSequence->TempoTime = 0;
	lpSequence->TempoTicks = 0;
	lpSequence->CurrentTicks = 0 ;
	lpSequence->CurrentTime = 0;
	lpSequence->TickInterval = 500000.0l / lpSequence->wBeatSize;
	lpSequence->qwLastTime.QuadPart = 0;
	for (i=0; i<lpSequence->wTracks; i++) lpSequence->Tracks[i].dwCurrentEvent = 0;
	MidiSilence((HANDLE) lpSequence);
	midiOutReset(lpSequence->Handle);

	return TRUE;
}

__declspec(dllexport) BOOL WINAPI MidiPlay(HANDLE hSequence)
{
	LPBRELS_MIDI_SEQUENCE lpSequence;
	DWORD old;

	if (IsBadReadPtr(hSequence, sizeof(BRELS_MIDI_SEQUENCE))) return FALSE;
	lpSequence = (LPBRELS_MIDI_SEQUENCE) hSequence;

	old = lpSequence->dwStatus;
	lpSequence->dwStatus = MIDI_PLAY;

	if (old!=MIDI_PLAY)
	{
		lpSequence->qwLastTime.QuadPart = 0;
		timeSetEvent((UINT)(lpSequence->Precision / 1000), (UINT)(lpSequence->Precision / 1000), PlayProc, (DWORD_PTR) hSequence, TIME_PERIODIC);
	}

	return TRUE;
}

__declspec(dllexport) BOOL WINAPI MidiPause(HANDLE hSequence)
{
	LPBRELS_MIDI_SEQUENCE lpSequence;

	if (IsBadReadPtr(hSequence, sizeof(BRELS_MIDI_SEQUENCE))) return FALSE;
	lpSequence = (LPBRELS_MIDI_SEQUENCE) hSequence;

	lpSequence->dwStatus = MIDI_PAUSE;
	lpSequence->Processed = FALSE;
	while (!lpSequence->Played) continue;
	MidiSilence(hSequence);

	PlayCallback(lpSequence, TRUE);

	return TRUE;
}

__declspec(dllexport) BOOL WINAPI MidiStop(HANDLE hSequence)
{
	LPBRELS_MIDI_SEQUENCE lpSequence;

	if (IsBadReadPtr(hSequence, sizeof(BRELS_MIDI_SEQUENCE))) return FALSE;
	lpSequence = (LPBRELS_MIDI_SEQUENCE) hSequence;

	lpSequence->dwStatus = MIDI_STOP;
	lpSequence->Processed = FALSE;
	while (!lpSequence->Played) continue;
	MidiReset(hSequence);

	PlayCallback(lpSequence, TRUE);

	return TRUE;
}


__declspec(dllexport) BOOL WINAPI MidiInsertTrack(HANDLE hSequence, WORD wTrack)
{
	LPBRELS_MIDI_SEQUENCE lpSequence;
	LPBRELS_MIDI_TRACK lpNew;

	if (IsBadReadPtr(hSequence, sizeof(BRELS_MIDI_SEQUENCE))) return FALSE;
	lpSequence = (LPBRELS_MIDI_SEQUENCE) hSequence;
	if (wTrack>lpSequence->wTracks) return FALSE;

	lpNew = (LPBRELS_MIDI_TRACK) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (lpSequence->wTracks+1)*sizeof(BRELS_MIDI_TRACK));

	lpNew[wTrack].Header.dwMagic = btldw(0x4D54726B);
	lpNew[wTrack].Header.dwLength = btldw(4);
	lpNew[wTrack].dwEvents = 1;
	lpNew[wTrack].dwLength = 4;
	lpNew[wTrack].dwCurrentEvent = 0;
	lpNew[wTrack].Mute = FALSE;
	lpNew[wTrack].Events = (LPBRELS_MIDI_EVENT) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BRELS_MIDI_EVENT));
	lpNew[wTrack].Events[0].wTag = 0;
	lpNew[wTrack].Events[0].dwTicks = 0;
	lpNew[wTrack].Events[0].Event = 0x2F;
	lpNew[wTrack].Events[0].DataSize = 0;
	lpNew[wTrack].Events[0].Data.p = NULL;

	if (wTrack>0) CopyMemory(lpNew, lpSequence->Tracks, sizeof(BRELS_MIDI_TRACK)*wTrack);
	if (wTrack<lpSequence->wTracks) CopyMemory(&lpNew[wTrack+1], &lpSequence->Tracks[wTrack], sizeof(BRELS_MIDI_TRACK)*(lpSequence->wTracks-wTrack));

	HeapFree(GetProcessHeap(), 0, lpSequence->Tracks);
	lpSequence->Tracks = lpNew;
	lpSequence->wTracks++;
	lpSequence->Header.wTracks = btlw(lpSequence->wTracks);
	lpSequence->dwEvents += 1;

	BuildTempoTable(lpSequence);

	return TRUE;
}

__declspec(dllexport) BOOL WINAPI MidiRemoveTrack(HANDLE hSequence, WORD wTrack)
{
	int i, New;
	LPBRELS_MIDI_SEQUENCE lpSequence;
	LPBRELS_MIDI_TRACK lpNew;

	if (IsBadReadPtr(hSequence, sizeof(BRELS_MIDI_SEQUENCE))) return FALSE;
	lpSequence = (LPBRELS_MIDI_SEQUENCE) hSequence;
	if (wTrack>=lpSequence->wTracks) return FALSE;

	lpNew = (LPBRELS_MIDI_TRACK) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (lpSequence->wTracks-1)*sizeof(BRELS_MIDI_TRACK));

	lpSequence->dwEvents -= lpSequence->Tracks[wTrack].dwEvents;

	New=0;
	for (i=0; i<lpSequence->wTracks; i++)
		if (i!=wTrack)
			lpNew[New++] = lpSequence->Tracks[i];


	MidiCleanEvents(lpSequence->Tracks[wTrack].dwEvents, lpSequence->Tracks[wTrack].Events);
	HeapFree(GetProcessHeap(), 0, lpSequence->Tracks);
	lpSequence->Tracks = lpNew;
	lpSequence->wTracks--;
	lpSequence->Header.wTracks = btlw(lpSequence->wTracks);

	BuildTempoTable(lpSequence);

	return TRUE;
}

int Compare(void* data1, void* data2)
{
	BYTE Value1IsMeta, Value2IsMeta;
	LPBRELS_MIDI_EVENT Event1, Event2;
	Event1 = (LPBRELS_MIDI_EVENT) data1;
	Event2 = (LPBRELS_MIDI_EVENT) data2;
	Value1IsMeta = (Event1->Event <0x80) || (Event1->Event > 0x9F);
	Value2IsMeta = (Event2->Event <0x80) || (Event2->Event > 0x9F);

	if (Event1->dwTicks == Event2->dwTicks)
		if (Event1->Event == Event2->Event)
			if (Event1->Data.a[0] == Event2->Data.a[0])
				return 1-2*(Event1->Data.a[1] < Event2->Data.a[1]);
			else
				return 1-2*(Event1->Data.a[0] < Event2->Data.a[0]);
		else
			if (Value1IsMeta==Value2IsMeta) /* Same nature, direct order: FF -> Notes / TT -> Metas */
				return 1-2*(Event1->Event < Event2->Event);
			else
				return 1-2*(Value1IsMeta > Value2IsMeta);
	else
		return 1-2*(Event1->dwTicks < Event2->dwTicks);
}

__declspec(dllexport) DWORD WINAPI MidiInsertTrackEvents(HANDLE hSequence, WORD wTrack, DWORD dwEvents, LPCBRELS_MIDI_EVENT lpEvents)
{
	int i, Offset, Ticks;
	DWORD nEvents;
	LPBRELS_MIDI_SEQUENCE lpSequence;
	LPBRELS_MIDI_EVENT lpNew;

	lpSequence = (LPBRELS_MIDI_SEQUENCE) hSequence;
	if (IsBadReadPtr(hSequence, sizeof(BRELS_MIDI_SEQUENCE))) return 0;
	/*if (IsBadCodePtr((FARPROC) &lpSequence->Tracks[wTrack])) return 0;
	if (IsBadCodePtr((FARPROC) lpEvents)) return 0;*/
	if (wTrack>=lpSequence->wTracks) return 0;

	nEvents = lpSequence->Tracks[wTrack].dwEvents + dwEvents;
	lpNew  = (LPBRELS_MIDI_EVENT) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nEvents * sizeof(BRELS_MIDI_EVENT));

	Offset = 0;
	/* Filter the track */
	for (i=0; i<(int)lpSequence->Tracks[wTrack].dwEvents; i++)
		if (lpSequence->Tracks[wTrack].Events[i].Event!=0x2F)
		{
			lpNew[Offset] = lpSequence->Tracks[wTrack].Events[i];
			Offset++;
		}
	for (i=0; i<(int) dwEvents; i++)
		if (lpEvents[i].Event!=0x2F)
		{
			lpNew[Offset] = lpEvents[i];
			if ((lpNew[Offset].Event < 0x80) || (lpNew[Offset].Event==0xF0))
			{
				lpNew[Offset].Data.p = (LPBYTE) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpNew[Offset].DataSize);
				CopyMemory(lpNew[Offset].Data.p, lpEvents[i].Data.p, lpNew[Offset].DataSize);
			}
			Offset++;
		}
	qsort(lpNew, Offset, sizeof(BRELS_MIDI_EVENT), (int(*)(const void*, const void*)) &Compare);

	/*Appends the end of track */
	Ticks = 0;
	if (Offset>0) Ticks = lpNew[Offset-1].dwTicks;
	lpNew[Offset].Event = 0x2F;
	lpNew[Offset].dwTicks = Ticks;
	lpNew[Offset].DataSize = 0;
	lpNew[Offset].Data.p = NULL;
	Offset++;

	HeapFree(GetProcessHeap(), 0, lpSequence->Tracks[wTrack].Events);
	lpSequence->Tracks[wTrack].Events = (LPBRELS_MIDI_EVENT) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Offset*sizeof(BRELS_MIDI_EVENT));
	CopyMemory(lpSequence->Tracks[wTrack].Events, lpNew, sizeof(BRELS_MIDI_EVENT)*Offset);
	HeapFree(GetProcessHeap(), 0, lpNew);

	lpSequence->dwEvents -= lpSequence->Tracks[wTrack].dwEvents;
	lpSequence->Tracks[wTrack].dwEvents = Offset;
	lpSequence->dwEvents += Offset;

	BuildTempoTable(lpSequence);

	if (!lpSequence->Suspended)
		AdjustPosition(lpSequence, wTrack);

	return Offset;
}

__declspec(dllexport) DWORD WINAPI MidiRemoveTrackEvents(HANDLE hSequence, WORD wTrack, QWORD qwFirst, QWORD qwLast, DWORD dwFormat)
{
	int Ticks;
	SSIZE_T Final, Inicial, Affected;
	SIZE_T i;
	LPBRELS_MIDI_SEQUENCE lpSequence;
	LPBRELS_MIDI_EVENT lpNew;

	lpSequence = (LPBRELS_MIDI_SEQUENCE) hSequence;
	if (IsBadReadPtr(hSequence, sizeof(BRELS_MIDI_SEQUENCE))) return 0;
	/*if (IsBadCodePtr((FARPROC) &lpSequence->Tracks[wTrack])) return 0;*/
	if (qwFirst>qwLast) return 0;
	if (wTrack>=lpSequence->wTracks) return 0;

	Final=-1;
	Inicial=-1;

	switch (dwFormat)
	{
	case MIDI_INDEX:
		if ((qwFirst<lpSequence->Tracks[wTrack].dwEvents) && (qwLast<lpSequence->Tracks[wTrack].dwEvents))
		{
			Inicial = (SSIZE_T) qwFirst;
			Final = (SSIZE_T) qwLast;
		}
		else
		{
			return 0;
		}
		break;
	case MIDI_TICKS:
		for (i=0; i<lpSequence->Tracks[wTrack].dwEvents; i++)
		{
			if (lpSequence->Tracks[wTrack].Events[lpSequence->Tracks[wTrack].dwEvents-i-1].dwTicks>=qwFirst) Inicial = lpSequence->Tracks[wTrack].dwEvents-i-1;
			if (lpSequence->Tracks[wTrack].Events[i].dwTicks<=qwLast) Final = i;
		}
		break;
	default:
		return 0;
	}

	if ((Final==-1) || (Inicial==-1))
	{
		return 0;
	}

	Affected = 0;
	lpNew = (LPBRELS_MIDI_EVENT) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (lpSequence->Tracks[wTrack].dwEvents-(Final-Inicial+1)+1) * sizeof(BRELS_MIDI_EVENT));
	if (Inicial>0)
	{
		CopyMemory(lpNew, lpSequence->Tracks[wTrack].Events, Inicial*sizeof(BRELS_MIDI_EVENT));
		Affected+=Inicial;
	}
	if (Final<(((int) lpSequence->Tracks[wTrack].dwEvents)-1))
	{
		CopyMemory(&lpNew[Inicial], &lpSequence->Tracks[wTrack].Events[Final+1], (lpSequence->Tracks[wTrack].dwEvents-Final)*sizeof(BRELS_MIDI_EVENT));
		Affected+=lpSequence->Tracks[wTrack].dwEvents-1-Final;
	}

	if (lpSequence->Tracks[wTrack].Events[Final].Event==0x2F)
	{
		if (Affected==0) Ticks = 0; else Ticks = lpNew[Affected-1].dwTicks;
		lpNew[Affected].Event=0x2F;
		lpNew[Affected].dwTicks=Ticks;
		lpNew[Affected].DataSize=0;
		lpNew[Affected].Data.p=NULL;
		Affected++;
	}

	for (i=Inicial; i<=(SIZE_T) Final; i++)
	{
		if ((lpSequence->Tracks[wTrack].Events[i].Event<0x80) || (lpSequence->Tracks[wTrack].Events[i].Event==0xf0))
			if (lpSequence->Tracks[wTrack].Events[i].DataSize>0)
				HeapFree(GetProcessHeap(), 0, lpSequence->Tracks[wTrack].Events[i].Data.p);
	}

	HeapFree(GetProcessHeap(), 0, lpSequence->Tracks[wTrack].Events);
	lpSequence->Tracks[wTrack].Events = lpNew;

	lpSequence->dwEvents = (DWORD)(lpSequence->dwEvents - lpSequence->Tracks[wTrack].dwEvents + Affected);
	lpSequence->Tracks[wTrack].dwEvents = (DWORD) Affected;


	/* Ajusta o fim de trilha */
	Ticks = 0;
	if (Affected>2) Ticks = lpSequence->Tracks[wTrack].Events[Affected-2].dwTicks;
	lpSequence->Tracks[wTrack].Events[Affected-1].dwTicks = Ticks;

	BuildTempoTable(lpSequence);

	if (!lpSequence->Suspended)
		AdjustPosition(lpSequence, wTrack);

	if (Affected > MAXDWORD)
		return MAXDWORD;
	return (DWORD) Affected;
}

__declspec(dllexport) DWORD WINAPI MidiGetTrackEvents(HANDLE hSequence, WORD wTrack, QWORD qwFirst, QWORD qwLast, DWORD dwFormat, LPBRELS_MIDI_EVENT* lpEvents)
{
	int i, Final, Inicial;
	DWORD Affected;
	LPBRELS_MIDI_SEQUENCE lpSequence;
	LPBRELS_MIDI_EVENT lpNew;

	lpSequence = (LPBRELS_MIDI_SEQUENCE) hSequence;
	if (IsBadReadPtr(hSequence, sizeof(BRELS_MIDI_SEQUENCE))) return 0;
	/*if (IsBadCodePtr((FARPROC) &lpSequence->Tracks[wTrack])) return 0;*/
	if (wTrack>=lpSequence->wTracks) return 0;
	if (qwFirst>qwLast) return 0;

	Final=-1;
	Inicial=-1;

	switch (dwFormat)
	{
	case MIDI_INDEX:
		if ((qwFirst<lpSequence->Tracks[wTrack].dwEvents) && (qwLast<lpSequence->Tracks[wTrack].dwEvents))
		{
			Inicial = (int) qwFirst;
			Final = (int) qwLast;
		}
		else
		{
			return 0;
		}
		break;
	case MIDI_TICKS:
		for (i=0; i<(int) lpSequence->Tracks[wTrack].dwEvents; i++)
		{
			if (lpSequence->Tracks[wTrack].Events[lpSequence->Tracks[wTrack].dwEvents-i-1].dwTicks>=qwFirst) Inicial = lpSequence->Tracks[wTrack].dwEvents-i-1;
			if (lpSequence->Tracks[wTrack].Events[i].dwTicks<=qwLast) Final = i;
		}
		break;
	default:
		return 0;
	}

	if ((Final==-1) || (Inicial==-1))
	{
		return 0;
	}

	Affected = Final - Inicial + 1;

	if (lpEvents!=NULL)
	{
		lpNew = (LPBRELS_MIDI_EVENT) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Affected*sizeof(BRELS_MIDI_EVENT));
		CopyMemory(lpNew, &lpSequence->Tracks[wTrack].Events[Inicial], Affected*sizeof(BRELS_MIDI_EVENT));
		for (i=Inicial; i<=Final; i++)
			if ((lpNew[i-Inicial].Event<0x80) || (lpNew[i-Inicial].Event==0xF0))
			{
				lpNew[i-Inicial].Data.p = (LPBYTE) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpNew[i-Inicial].DataSize);
				CopyMemory(lpNew[i-Inicial].Data.p, lpSequence->Tracks[wTrack].Events[i].Data.p, lpNew[i-Inicial].DataSize);
			}
		*lpEvents = lpNew;
	}

	return Affected;
}

__declspec(dllexport) DWORD WINAPI MidiFilterTrackEvents(HANDLE hSequence, WORD wTrack, QWORD qwFirst, QWORD qwLast, DWORD dwFormat, QWORD qwFilter, LPBRELS_FILTERED_EVENT* lpFilteredEvents)
{
	int i, j, k, Final, Inicial;
	int Key, Count[128], Velocity, EarlyTempo;
	int PendIndex[128][128];
	int PendCount[128];
	DWORD Offset;
	BOOL Tempo, Notes;
	QWORD Filters[256];
	LPBRELS_MIDI_SEQUENCE lpSequence;
	LPBRELS_FILTERED_EVENT lpNew, lpTemp;
	BYTE Event;

	lpSequence = (LPBRELS_MIDI_SEQUENCE) hSequence;
	if (IsBadReadPtr(hSequence, sizeof(BRELS_MIDI_SEQUENCE))) return 0;
	/*if (IsBadCodePtr((FARPROC) &lpSequence->Tracks[wTrack])) return 0;*/
	if (wTrack>=lpSequence->wTracks) return 0;
	if (qwFirst > qwLast) return 0;

	Final=-1;
	Inicial=-1;

	switch (dwFormat)
	{
	case MIDI_INDEX:
		if ((qwFirst<lpSequence->Tracks[wTrack].dwEvents) && (qwLast<lpSequence->Tracks[wTrack].dwEvents))
		{
			Inicial = (int) qwFirst;
			Final = (int) qwLast;
		}
		else
		{
			return 0;
		}
		break;
	case MIDI_TICKS:
		for (i=0; i<(int) lpSequence->Tracks[wTrack].dwEvents; i++)
		{
			if (lpSequence->Tracks[wTrack].Events[lpSequence->Tracks[wTrack].dwEvents-i-1].dwTicks>=qwFirst) Inicial = lpSequence->Tracks[wTrack].dwEvents-i-1;
			if (lpSequence->Tracks[wTrack].Events[i].dwTicks<=qwLast) Final = i;
		}
		break;
	default:
		return 0;
	}


	if ((Inicial==-1) || (Final==-1))
	{
		return 0;
	}


	Offset=0;
	lpTemp = (LPBRELS_FILTERED_EVENT) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 2*lpSequence->Tracks[wTrack].dwEvents*sizeof(BRELS_FILTERED_EVENT));
	ZeroMemory(&PendIndex, sizeof(PendIndex));
	ZeroMemory(&PendCount, sizeof(PendCount));

	ZeroMemory(&Filters, sizeof(Filters));
	for (i=0; i<256; i++) Filters[i]=qwFilter & FILTER_OTHER;
	Filters[0x80] = qwFilter & FILTER_NOTES ? 0 : (qwFilter & FILTER_ALL) ? FILTER_NOTEOFF:              (qwFilter & FILTER_NOTEOFF)            ;
	Filters[0x90] = qwFilter & FILTER_NOTES ? 0 : (qwFilter & FILTER_ALL) ? FILTER_NOTEON :              (qwFilter & FILTER_NOTEON )            ;
	Filters[0xA0] = (qwFilter & FILTER_ALL) ? FILTER_KEYAFTERTOUCH:        (qwFilter & FILTER_KEYAFTERTOUCH)      ;
	Filters[0xB0] = (qwFilter & FILTER_ALL) ? FILTER_CONTROLLER:           (qwFilter & FILTER_CONTROLLER)         ;
	Filters[0xC0] = (qwFilter & FILTER_ALL) ? FILTER_PROGRAM      :        (qwFilter & FILTER_PROGRAM)            ;
	Filters[0xD0] = (qwFilter & FILTER_ALL) ? FILTER_CHANNELPRESSURE:      (qwFilter & FILTER_CHANNELPRESSURE)    ;
	Filters[0xE0] = (qwFilter & FILTER_ALL) ? FILTER_PITCHWHEEL:           (qwFilter & FILTER_PITCHWHEEL)         ;
	Filters[0x00] = (qwFilter & FILTER_ALL) ? FILTER_SEQUENCENUMBER:       (qwFilter & FILTER_SEQUENCENUMBER)     ;
	Filters[0x01] = (qwFilter & FILTER_ALL) ? FILTER_TEXT:                 (qwFilter & FILTER_TEXT)               ;
	Filters[0x02] = (qwFilter & FILTER_ALL) ? FILTER_COPYRIGHT:            (qwFilter & FILTER_COPYRIGHT)          ;
	Filters[0x03] = (qwFilter & FILTER_ALL) ? FILTER_TRACKNAME:            (qwFilter & FILTER_TRACKNAME)          ;
	Filters[0x04] = (qwFilter & FILTER_ALL) ? FILTER_INSTRUMENTNAME:       (qwFilter & FILTER_INSTRUMENTNAME)     ;
	Filters[0x05] = (qwFilter & FILTER_ALL) ? FILTER_LYRIC:                (qwFilter & FILTER_LYRIC)              ;
	Filters[0x06] = (qwFilter & FILTER_ALL) ? FILTER_MARKER:               (qwFilter & FILTER_MARKER)             ;
	Filters[0x07] = (qwFilter & FILTER_ALL) ? FILTER_CUEPOINT:             (qwFilter & FILTER_CUEPOINT)           ;
	Filters[0x20] = (qwFilter & FILTER_ALL) ? FILTER_CHANNELPREFIX:        (qwFilter & FILTER_CHANNELPREFIX)      ;
	Filters[0x2F] = (qwFilter & FILTER_ALL) ? FILTER_ENDOFTRACK:           (qwFilter & FILTER_ENDOFTRACK)         ;
	Filters[0x54] = (qwFilter & FILTER_ALL) ? FILTER_SMPTEOFFSET:          (qwFilter & FILTER_SMPTEOFFSET)        ;
	Filters[0x58] = (qwFilter & FILTER_ALL) ? FILTER_TIMESIGNATURE:        (qwFilter & FILTER_TIMESIGNATURE)      ;
	Filters[0x59] = (qwFilter & FILTER_ALL) ? FILTER_KEYSIGNATURE:         (qwFilter & FILTER_KEYSIGNATURE)       ;
	Filters[0x7F] = (qwFilter & FILTER_ALL) ? FILTER_SEQUENCERSPECIFIC:    (qwFilter & FILTER_SEQUENCERSPECIFIC)  ;
	Filters[0xF0] = (qwFilter & FILTER_ALL) ? FILTER_SYSTEMEXCLUSIVE:      (qwFilter & FILTER_SYSTEMEXCLUSIVE)    ;
	Filters[0xF1] = (qwFilter & FILTER_ALL) ? FILTER_MTCQUARTERFRAME:      (qwFilter & FILTER_MTCQUARTERFRAME)    ;
	Filters[0xF2] = (qwFilter & FILTER_ALL) ? FILTER_SONGPOSITIONPOINTER : (qwFilter & FILTER_SONGPOSITIONPOINTER);
	Filters[0xF3] = (qwFilter & FILTER_ALL) ? FILTER_SONGSELECT:           (qwFilter & FILTER_SONGSELECT)         ;
	Filters[0xF6] = (qwFilter & FILTER_ALL) ? FILTER_TUNEREQUEST:          (qwFilter & FILTER_TUNEREQUEST)        ;
	/*
	Filters[0xF8] = (qwFilter & FILTER_ALL) ? FILTER_MIDICLOCK:            (qwFilter & FILTER_MIDICLOCK)          ;
	Filters[0xFA] = (qwFilter & FILTER_ALL) ? FILTER_MIDISTART:            (qwFilter & FILTER_MIDISTART)          ;
	Filters[0xFB] = (qwFilter & FILTER_ALL) ? FILTER_MIDICONTINUE:         (qwFilter & FILTER_MIDICONTINUE)       ;
	Filters[0xFC] = (qwFilter & FILTER_ALL) ? FILTER_MIDISTOP:             (qwFilter & FILTER_MIDISTOP)           ;
	*/
	Filters[0xFE] = (qwFilter & FILTER_ALL) ? FILTER_ACTIVESENSE:          (qwFilter & FILTER_ACTIVESENSE)        ;
	Tempo         = (qwFilter & FILTER_TEMPO) == FILTER_TEMPO;
	Notes         = (qwFilter & FILTER_NOTES) == FILTER_NOTES;

	ZeroMemory(&Count, sizeof(Count));
	EarlyTempo = -1;

	for (i=Inicial; i<=Final; i++)
	{
		Event = lpSequence->Tracks[wTrack].Events[i].Event;
		if ((Event>=0x80) && (Event<=0xEF)) Event&=0xF0;
		if (Filters[Event])
		{
			lpTemp[Offset].qwFilter = Filters[Event];
			lpTemp[Offset].qwStart = i;
			lpTemp[Offset].qwEnd = i;
			Offset++;
		}

		if (Notes)
		{
			Key = lpSequence->Tracks[wTrack].Events[i].Data.a[0];
			Velocity = lpSequence->Tracks[wTrack].Events[i].Data.a[1];
			if (Event==0x90)
			{
				if (Velocity)
				{
					if (PendCount[Key]<128)
					{
						PendIndex[Key][PendCount[Key]++] = Offset;
						lpTemp[Offset].qwFilter=FILTER_NOTES;
						lpTemp[Offset].qwStart = i;
						lpTemp[Offset].qwEnd = lpSequence->Tracks[wTrack].dwEvents-1;
						Offset++;
					}
				}
				else
				if (PendCount[Key]>0)
				{
					lpTemp[Offset].qwFilter=FILTER_NOTES;
					lpTemp[PendIndex[Key][0]].qwEnd = i;
					PendCount[Key]--;
					for (j=0; j<PendCount[Key]; j++) PendIndex[Key][j]=PendIndex[Key][j+1];
				}
			}

			if (Event==0x80)
				if (PendCount[Key]>0)
				{
					lpTemp[Offset].qwFilter=FILTER_NOTES;
					lpTemp[PendIndex[Key][0]].qwEnd = i;
					PendCount[Key]--;
					for (j=0; j<PendCount[Key]; j++) PendIndex[Key][j]=PendIndex[Key][j+1];
				}
		}

		if (Tempo)
		if (Event==0x51)
		{
			lpTemp[Offset].qwFilter=FILTER_TEMPO;
			k = lpSequence->Tracks[wTrack].dwEvents-1;
			lpTemp[Offset].qwStart = i;
			lpTemp[Offset].qwEnd = k;
			if (EarlyTempo!=-1) lpTemp[EarlyTempo].qwEnd = i;
			EarlyTempo = Offset++;
		}
	}

	if (lpFilteredEvents!=NULL)
	{
		lpNew = (LPBRELS_FILTERED_EVENT) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Offset*sizeof(BRELS_FILTERED_EVENT));
		CopyMemory(lpNew, lpTemp, Offset*sizeof(BRELS_FILTERED_EVENT));
		*lpFilteredEvents = lpNew;
	}

	HeapFree(GetProcessHeap(), 0, lpTemp);

	return Offset;
}

__declspec(dllexport) DWORD WINAPI MidiCleanEvents(DWORD dwEvents, LPBRELS_MIDI_EVENT lpEvents)
{
	int i;
	DWORD Affected;

	/*if (IsBadCodePtr((FARPROC) lpEvents)) return 0;*/

	Affected = 0;

	for (i=0; i<(int) dwEvents; i++)
	{
		Affected++;
		if ((lpEvents[i].Event<0x80) || (lpEvents[i].Event==0xF0))
		if (lpEvents[i].DataSize>0)
		/*if (!IsBadCodePtr((FARPROC) lpEvents[i].lpData))*/
			HeapFree(GetProcessHeap(), 0, lpEvents[i].Data.p);
	}

	HeapFree(GetProcessHeap(), 0, lpEvents);

	return Affected;
}

__declspec(dllexport) BOOL WINAPI MidiFreeBuffer(LPVOID lpBuffer)
{
	/*if (IsBadCodePtr((FARPROC) lpBuffer)) return FALSE;*/
	return HeapFree(GetProcessHeap(), 0, lpBuffer);
}

__declspec(dllexport) QWORD WINAPI MidiGet(HANDLE hSequence, DWORD dwWhat)
{
	int i;
	QWORD Retorno;
	LPBRELS_MIDI_SEQUENCE lpSequence;
	LPBRELS_MIDI_HEADER lpbmh;

	lpSequence = (LPBRELS_MIDI_SEQUENCE) hSequence;
	if (IsBadReadPtr(hSequence, sizeof(BRELS_MIDI_SEQUENCE))) return BRELS_ERROR;

	switch (dwWhat)
	{
	case BEAT_SIZE      :
		Retorno = (QWORD) lpSequence->wBeatSize;
		break;
	case MIDI_DEVICE    :
		Retorno = (QWORD) lpSequence->dwDevice;
		break;
	case MIDI_HANDLE    :
		Retorno = (QWORD) (DWORD_PTR) lpSequence->Handle;
		break;
	case MIDI_HEADER    :
		lpbmh = (LPBRELS_MIDI_HEADER) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BRELS_MIDI_HEADER));
		*lpbmh = lpSequence->Header;
		Retorno = (QWORD) (DWORD_PTR) lpbmh;
		break;
	case MIDI_LOOP      :
		Retorno = (QWORD) lpSequence->Loop;
		break;
	case MIDI_STATUS:
		if (lpSequence->Suspended)
			Retorno = (QWORD) MIDI_BUSY;
		else
			Retorno = (QWORD) lpSequence->dwStatus;
		break;
	case MIDI_PRECISION :
		Retorno = (QWORD) lpSequence->Precision;
		break;
	case MIDI_DONE:
		Retorno = (QWORD) lpSequence->Played;
		break;
	case CALLBACK_HWND:
		Retorno = (QWORD) (DWORD_PTR) lpSequence->Callback;
		break;
	case CALLBACK_MESSAGE:
		Retorno = (QWORD) lpSequence->CallbackMessage;
		break;
	case CALLBACK_PROCESSED:
		Retorno = (QWORD) lpSequence->Processed;
		break;
	case SOLO_TRACK     :
		Retorno = (QWORD) lpSequence->SoloTrack;
		break;
	case BYTE_COUNT     :
		Retorno = (QWORD) sizeof(BRELS_MIDI_HEADER);
		for (i=0; i<lpSequence->wTracks; i++)
			Retorno += (QWORD) (sizeof(BRELS_TRACK_HEADER)  + GetTrackSize(lpSequence, i));
		break;
	case TICK_COUNT     :
		Retorno = 0;
		for (i=0; i<lpSequence->wTracks; i++)
			if (Retorno < (QWORD) lpSequence->Tracks[i].Events[lpSequence->Tracks[i].dwEvents-1].dwTicks)
				Retorno = (QWORD) lpSequence->Tracks[i].Events[lpSequence->Tracks[i].dwEvents-1].dwTicks;
		break;
	case TIME_COUNT     :
		Retorno = 0;
		for (i=0; i<lpSequence->wTracks; i++)
			if (Retorno < (QWORD) lpSequence->Tracks[i].Events[lpSequence->Tracks[i].dwEvents-1].dwTicks)
				Retorno = (QWORD) lpSequence->Tracks[i].Events[lpSequence->Tracks[i].dwEvents-1].dwTicks;
		Retorno = MidiQuery(hSequence, 0, MIDI_TIME, MIDI_TICKS, Retorno);
		break;
	case EVENT_COUNT    :
		Retorno = (QWORD) lpSequence->dwEvents;
		break;
	case TRACK_COUNT    :
		Retorno = (QWORD) lpSequence->wTracks;
		break;
	case CURRENT_TIME   :
		Retorno = (QWORD) lpSequence->CurrentTime;
		break;
	case CURRENT_TICKS  :
		Retorno = (QWORD) lpSequence->CurrentTicks;
		break;
	case CURRENT_TEMPO  :
		Retorno = (QWORD) lpSequence->dwCurrentTempo;
		break;
	case TICK_INTERVAL  :
		Retorno = (QWORD) lpSequence->TickInterval;
		break;
	case SKIP_INTERVAL  :
		Retorno = (QWORD) lpSequence->Skipped;
		break;
	default:
		Retorno = BRELS_ERROR;
	}

	return Retorno;
}

__declspec(dllexport) QWORD WINAPI MidiTrackGet(HANDLE hSequence, WORD wTrack, DWORD dwWhat)
{
	QWORD Retorno;
	QWORD* temp;
	LPBRELS_TRACK_HEADER lpth;
	LPBRELS_MIDI_SEQUENCE lpSequence;

	lpSequence = (LPBRELS_MIDI_SEQUENCE) hSequence;
	if (IsBadReadPtr(hSequence, sizeof(BRELS_MIDI_SEQUENCE))) return BRELS_ERROR;
	/*if (IsBadCodePtr((FARPROC) &lpSequence->Tracks[wTrack])) return BRELS_ERROR;*/
	if (wTrack>=lpSequence->wTracks) return BRELS_ERROR;

	switch (dwWhat)
	{
	case BYTE_COUNT:
		Retorno = (QWORD) GetTrackSize(lpSequence, wTrack);
		break;
	case TICK_COUNT:
		Retorno = (QWORD) lpSequence->Tracks[wTrack].Events[lpSequence->Tracks[wTrack].dwEvents-1].dwTicks;
		break;
	case TIME_COUNT:
		Retorno = MidiQuery(hSequence, wTrack, MIDI_TIME, MIDI_TICKS, lpSequence->Tracks[wTrack].Events[lpSequence->Tracks[wTrack].dwEvents-1].dwTicks);
		break;
	case EVENT_COUNT:
		Retorno = (QWORD) lpSequence->Tracks[wTrack].dwEvents;
		break;
	case CURRENT_EVENT:
		Retorno = (QWORD) lpSequence->Tracks[wTrack].dwCurrentEvent;
		break;
	case TRACK_DATA:
		temp = &Retorno;
		GetTrackData(lpSequence, wTrack, (LPBYTE*) temp);
		break;
	case TRACK_HEADER:
		lpth = (LPBRELS_TRACK_HEADER) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BRELS_TRACK_HEADER));
		lpSequence->Tracks[wTrack].Header.dwLength = btldw(GetTrackSize(lpSequence, wTrack));
		*lpth = lpSequence->Tracks[wTrack].Header;
		Retorno = (QWORD) (DWORD_PTR) lpth;
		break;
	case TRACK_MUTE:
		Retorno = (QWORD) lpSequence->Tracks[wTrack].Mute;
		break;
	default:
		Retorno = BRELS_ERROR;
	}

	return Retorno;
}

__declspec(dllexport) QWORD WINAPI MidiEventGet(HANDLE hSequence, WORD wTrack, DWORD dwEvent, DWORD dwWhat)
{
	QWORD Retorno;
	LPBYTE lpBuffer;
	LPBRELS_MIDI_SEQUENCE lpSequence;

	lpSequence = (LPBRELS_MIDI_SEQUENCE) hSequence;
	if (IsBadReadPtr(hSequence, sizeof(BRELS_MIDI_SEQUENCE))) return BRELS_ERROR;
	/*if (IsBadCodePtr((FARPROC) &lpSequence->Tracks[wTrack])) return BRELS_ERROR;
	if (IsBadCodePtr((FARPROC) &lpSequence->Tracks[wTrack].Events[dwEvent])) return BRELS_ERROR;*/

	switch (dwWhat)
	{
	case EVENT_INDEX:
		Retorno = (QWORD) dwEvent;
		break;
	case EVENT_TIME:
		Retorno = MidiQuery(hSequence, wTrack, MIDI_TIME, MIDI_TICKS, (QWORD) lpSequence->Tracks[wTrack].Events[dwEvent].dwTicks);
		break;
	case EVENT_TICKS:
		Retorno = (QWORD) lpSequence->Tracks[wTrack].Events[dwEvent].dwTicks;
		break;
	case EVENT_EVENT:
		Retorno = (QWORD) lpSequence->Tracks[wTrack].Events[dwEvent].Event;
		break;
	case EVENT_DATA:
		lpBuffer = (LPBYTE) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpSequence->Tracks[wTrack].Events[dwEvent].DataSize);
		if ((lpSequence->Tracks[wTrack].Events[dwEvent].Event<0x80) || (lpSequence->Tracks[wTrack].Events[dwEvent].Event==0xF0))
			CopyMemory(lpBuffer, lpSequence->Tracks[wTrack].Events[dwEvent].Data.p, lpSequence->Tracks[wTrack].Events[dwEvent].DataSize);
		else
			CopyMemory(lpBuffer, &lpSequence->Tracks[wTrack].Events[dwEvent].Data, lpSequence->Tracks[wTrack].Events[dwEvent].DataSize);
		Retorno = (QWORD) (DWORD_PTR) lpBuffer;
		break;
	case EVENT_DATASIZE:
		Retorno = (QWORD) lpSequence->Tracks[wTrack].Events[dwEvent].DataSize;
		break;
	case EVENT_TAG:
		Retorno = (QWORD) lpSequence->Tracks[wTrack].Events[dwEvent].wTag;
		break;
	default:
		return BRELS_ERROR;
	}

	return Retorno;
}

__declspec(dllexport) QWORD WINAPI MidiSet(HANDLE hSequence, DWORD dwWhat, QWORD qwValue)
{
	LPBRELS_MIDI_SEQUENCE lpSequence;
	WORD newDelta;
	LPBRELS_MIDI_EVENT Event;
	QWORD Retorno;
	int i;
	int Channels[16];

	lpSequence = (LPBRELS_MIDI_SEQUENCE) hSequence;
	if (IsBadReadPtr(hSequence, sizeof(BRELS_MIDI_SEQUENCE))) return BRELS_ERROR;

	Retorno = BRELS_ERROR;

	switch (dwWhat)
	{
	case BEAT_SIZE:
		newDelta = (WORD) qwValue;
		if (newDelta==0) return BRELS_ERROR;
		Retorno = (QWORD) lpSequence->wBeatSize;
		lpSequence->wBeatSize = newDelta;
		lpSequence->Header.wBeatSize = btlw(newDelta);
		break;
	case CURRENT_TEMPO:
		Retorno = (QWORD) lpSequence->dwCurrentTempo;
		lpSequence->dwCurrentTempo = (DWORD) qwValue;
		lpSequence->TempoTicks = lpSequence->dwCurrentTempo;
		lpSequence->TempoTime = lpSequence->CurrentTime;
		lpSequence->TickInterval = lpSequence->dwCurrentTempo / lpSequence->wBeatSize;
		break;
	case MIDI_HANDLE:
		midiOutClose(lpSequence->Handle);
		lpSequence->Handle = (HMIDIOUT) (DWORD_PTR) qwValue;
		Retorno = (QWORD) TRUE;
		break;
	case MIDI_LOOP:
		Retorno = (QWORD) lpSequence->Loop;
		lpSequence->Loop = (BOOL) qwValue;
		break;
	case MIDI_PRECISION :
		Retorno = (QWORD) lpSequence->Precision;
		if (qwValue < 1000) return BRELS_ERROR;
		lpSequence->Precision = qwValue;
		break;
	case CALLBACK_HWND:
		Retorno = (QWORD) (DWORD_PTR) lpSequence->Callback;
		lpSequence->Callback = (HWND) (DWORD_PTR) qwValue;
		break;
	case CALLBACK_MESSAGE:
		Retorno = (QWORD) lpSequence->CallbackMessage;
		lpSequence->CallbackMessage = (UINT) qwValue;
		break;
	case CALLBACK_PROCESSED:
		Retorno = (QWORD) lpSequence->Processed;
		lpSequence->Processed = (UINT) qwValue;
		break;
	case SOLO_TRACK:
		if ((qwValue >= lpSequence->wTracks) && (qwValue!=NO_SOLO)) return BRELS_ERROR;
		Retorno = (QWORD) lpSequence->SoloTrack;
		lpSequence->SoloTrack = (DWORD) qwValue;
		if (qwValue!=NO_SOLO)
		{
			for (i=0; i<16; i++) Channels[i]=-1;
			for (i=0; i<(int) lpSequence->Tracks[qwValue].dwEvents; i++)
				if ((lpSequence->Tracks[qwValue].Events[i].Event & 0xF0)==0x90)
					Channels[lpSequence->Tracks[qwValue].Events[i].Event & 0x0F] = 1;
			for (i=0; i<16; i++)
				if (Channels[i]==-1)
					midiOutShortMsg(lpSequence->Handle, (0xB0 | i) | (120 << 8));
		}
		break;
	case CURRENT_TIME:
		Retorno = (QWORD) lpSequence->CurrentTime;
		MidiSet(hSequence, CURRENT_TICKS, MidiQuery(hSequence, 0, MIDI_TICKS, MIDI_TIME, qwValue));
		break;
	case CURRENT_TICKS:
		Retorno = (QWORD) lpSequence->CurrentTicks;

		lpSequence->CurrentTicks = (double) qwValue;
		lpSequence->CurrentTime = (double) MidiQuery(hSequence, 0, MIDI_TIME, MIDI_TICKS, qwValue);

		for (i=0; i<lpSequence->wTracks; i++)
			AdjustPosition(lpSequence, i);

		for (i=0; i<(int) lpSequence->dwTempos; i++)
		{
			Event = &lpSequence->Tracks[lpSequence->lpTempos[i].wTrack].Events[lpSequence->lpTempos[i].dwEvent];
			if (Event->dwTicks <= lpSequence->CurrentTicks)
			{
				lpSequence->dwCurrentTempo = (Event->Data.p[0] << 16) | (Event->Data.p[1] << 8) | Event->Data.p[2];
				lpSequence->TempoTime = (double) MidiQuery(hSequence, 0, MIDI_TIME, MIDI_TICKS, Event->dwTicks);
				lpSequence->TempoTicks = Event->dwTicks;
				lpSequence->TickInterval = lpSequence->dwCurrentTempo / lpSequence->wBeatSize;
			}
		}

		break;
	default:
		Retorno = BRELS_ERROR;
		break;
	}

	return Retorno;
}

__declspec(dllexport) QWORD WINAPI MidiTrackSet(HANDLE hSequence, WORD wTrack, DWORD dwWhat, QWORD qwValue)
{
	int i, Channels[16];
	QWORD Retorno;
	LPBRELS_MIDI_SEQUENCE lpSequence;
	BRELS_MIDI_TRACK Track;

	lpSequence = (LPBRELS_MIDI_SEQUENCE) hSequence;
	/*if (IsBadReadPtr(hSequence, sizeof(BRELS_MIDI_SEQUENCE))) return BRELS_ERROR;
	if (IsBadCodePtr((FARPROC) &lpSequence->Tracks[wTrack])) return BRELS_ERROR;*/

	Retorno = BRELS_ERROR;

	switch (dwWhat)
	{
	case CURRENT_EVENT:
		Retorno = (QWORD) lpSequence->Tracks[wTrack].dwCurrentEvent;
		lpSequence->Tracks[wTrack].dwCurrentEvent = (DWORD) qwValue;
		break;
	case TRACK_MUTE:
		Retorno = (QWORD) lpSequence->Tracks[wTrack].Mute;
		lpSequence->Tracks[wTrack].Mute = (BOOL) qwValue;
		if (qwValue)
		{
			for (i=0; i<16; i++) Channels[i]=-1;
			for (i=0; i<(int) lpSequence->Tracks[wTrack].dwEvents; i++)
				if ((lpSequence->Tracks[wTrack].Events[i].Event & 0xF0)==0x90)
					Channels[lpSequence->Tracks[wTrack].Events[i].Event & 0x0F] = 1;
			for (i=0; i<16; i++)
				if (Channels[i]==1)
					midiOutShortMsg(lpSequence->Handle, (0xB0 | i) | (120 << 8));
		}
		break;
	case TRACK_INDEX:
		/*if (IsBadCodePtr((FARPROC) &lpSequence->Tracks[qwValue])) return BRELS_ERROR;*/
		Retorno = wTrack;
		Track = lpSequence->Tracks[wTrack];
		lpSequence->Tracks[wTrack] = lpSequence->Tracks[qwValue];
		lpSequence->Tracks[qwValue] = Track;
		BuildTempoTable(lpSequence);
		break;
	default:
		Retorno = BRELS_ERROR;
		break;
	}

	return Retorno;
}

__declspec(dllexport) QWORD WINAPI MidiEventSet(HANDLE hSequence, WORD wTrack, DWORD dwEvent, DWORD dwWhat, QWORD qwValue)
{
	QWORD Retorno;
	LPBRELS_MIDI_SEQUENCE lpSequence;

	lpSequence = (LPBRELS_MIDI_SEQUENCE) hSequence;
	if (IsBadReadPtr(hSequence, sizeof(BRELS_MIDI_SEQUENCE))) return BRELS_ERROR;
	/*if (IsBadCodePtr((FARPROC) &lpSequence->Tracks[wTrack])) return BRELS_ERROR;
	if (IsBadCodePtr((FARPROC) &lpSequence->Tracks[wTrack].Events[dwEvent])) return BRELS_ERROR;*/

	Retorno = BRELS_ERROR;

	switch (dwWhat)
	{
	case EVENT_TAG:
		Retorno = (QWORD) lpSequence->Tracks[wTrack].Events[dwEvent].wTag;
		lpSequence->Tracks[wTrack].Events[dwEvent].wTag = (WORD) qwValue;
		break;
	default:
		Retorno = BRELS_ERROR;
		break;
	}

	return Retorno;
}


__declspec(dllexport) QWORD WINAPI MidiQuery(HANDLE hSequence, WORD wTrack, DWORD dwWhat, DWORD dwFrom, QWORD qwValue)
{
	DWORD i, Track, Event;
	QWORD Retorno;
	double dblTime, Tempo, Ticks, Delta, EventTicks, Value;
	LPBRELS_MIDI_SEQUENCE lpSequence;

	UNREFERENCED_PARAMETER(wTrack);

	lpSequence = (LPBRELS_MIDI_SEQUENCE) hSequence;
	if (IsBadReadPtr(hSequence, sizeof(BRELS_MIDI_SEQUENCE))) return BRELS_ERROR;

	Retorno = BRELS_ERROR;

	switch (dwWhat)
	{
	case MIDI_TIME:
		switch (dwFrom)
		{
		case MIDI_TICKS:
			dblTime = 0.0l;
			Ticks = 0.0l;
			Tempo = 500000.0l;
			Value = (double) qwValue;
			Delta = lpSequence->wBeatSize;
			for (i=0; i<lpSequence->dwTempos; i++)
			{
				Track = lpSequence->lpTempos[i].wTrack;
				Event = lpSequence->lpTempos[i].dwEvent;
				EventTicks = lpSequence->Tracks[Track].Events[Event].dwTicks;
				if (EventTicks> qwValue) break;
				dblTime = dblTime + (EventTicks - Ticks) * Tempo / Delta;
			 	Tempo = (lpSequence->Tracks[Track].Events[Event].Data.p[0] << 16) | (lpSequence->Tracks[Track].Events[Event].Data.p[1] << 8) | lpSequence->Tracks[Track].Events[Event].Data.p[2];
				Ticks = EventTicks;
			}
			Retorno = (QWORD)(dblTime + (Value - Ticks) * Tempo / Delta);
			break;
		default:
			Retorno = BRELS_ERROR;
			break;
		}
		break;
	case MIDI_TICKS:
		switch (dwFrom)
		{
		case MIDI_TIME:
			dblTime = 0.0l;
			Ticks = 0.0l;
			Tempo = 500000.0l;
			Delta = lpSequence->wBeatSize;
			Value = (double) qwValue;
			for (i=0; i<lpSequence->dwTempos; i++)
			{
				Track = lpSequence->lpTempos[i].wTrack;
				Event = lpSequence->lpTempos[i].dwEvent;
				EventTicks = lpSequence->Tracks[Track].Events[Event].dwTicks;
				if ((Value-dblTime)<((EventTicks - Ticks) * Tempo / Delta)) break;
				dblTime = dblTime + (EventTicks - Ticks) * Tempo / Delta;
			 	Tempo = (lpSequence->Tracks[Track].Events[Event].Data.p[0] << 16) | (lpSequence->Tracks[Track].Events[Event].Data.p[1] << 8) | lpSequence->Tracks[Track].Events[Event].Data.p[2];
				Ticks = EventTicks;
			}

			Retorno = (QWORD)(Ticks + (Value - dblTime) * Delta / Tempo);
			break;
		default:
			Retorno = BRELS_ERROR;
			break;
		}
		break;
	case MIDI_EVENT:
		switch (dwFrom)
		{
		case MIDI_FILTER:
			switch (qwValue)
			{
			case FILTER_SEQUENCENUMBER     : return 0x00;
			case FILTER_TEXT               : return 0x01;
			case FILTER_COPYRIGHT          : return 0x02;
			case FILTER_TRACKNAME          : return 0x03;
			case FILTER_INSTRUMENTNAME     : return 0x04;
			case FILTER_LYRIC              : return 0x05;
			case FILTER_MARKER             : return 0x06;
			case FILTER_CUEPOINT           : return 0x07;
			case FILTER_CHANNELPREFIX      : return 0x20;
			case FILTER_ENDOFTRACK         : return 0x2F;
			case FILTER_TEMPO              : return 0x51;
			case FILTER_SMPTEOFFSET        : return 0x54;
			case FILTER_TIMESIGNATURE      : return 0x58;
			case FILTER_KEYSIGNATURE       : return 0x59;
			case FILTER_SEQUENCERSPECIFIC  : return 0x7F;
			case FILTER_SYSTEMEXCLUSIVE    : return 0xF0;
			case FILTER_NOTEOFF            : return 0x80;
			case FILTER_NOTEON             : return 0x90;
			case FILTER_KEYAFTERTOUCH      : return 0xA0;
			case FILTER_CONTROLLER         : return 0xB0;
			case FILTER_PROGRAM            : return 0xC0;
			case FILTER_CHANNELPRESSURE    : return 0xD0;
			case FILTER_PITCHWHEEL         : return 0xE0;
			case FILTER_MTCQUARTERFRAME    : return 0xF1;
			case FILTER_SONGPOSITIONPOINTER: return 0xF2;
			case FILTER_SONGSELECT         : return 0xF3;
			case FILTER_TUNEREQUEST        : return 0xF6;
			/*
			case FILTER_MIDICLOCK          : return 0xF8;
			case FILTER_MIDISTART          : return 0xFA;
			case FILTER_MIDICONTINUE       : return 0xFB;
			case FILTER_MIDISTOP           : return 0xFC;
			*/
			case FILTER_ACTIVESENSE        : return 0xFE;
			default                        : return BRELS_ERROR;
			}
			break;
		default:
			Retorno = BRELS_ERROR;
			break;
		}
		break;
	default:
		Retorno = BRELS_ERROR;
		break;
	}

	return Retorno;
}

__declspec(dllexport) WORD WINAPI btlw(WORD big)
{
	WORD A, B;
	A = big >> 8;
	B = big & 0xFF;
	return (B << 8) | A ;
}

__declspec(dllexport) DWORD WINAPI btldw(DWORD big)
{
	DWORD A, B, C, D;
	A = (big >> 24) & 0xFF;
	B = (big >> 16) & 0xFF;
	C = (big >>  8) & 0xFF;
	D = (big      ) & 0xFF;
	return (D << 24) | (C << 16) | (B << 8) | A;
}

/* Helper functions */

void BuildTempoTable(LPBRELS_MIDI_SEQUENCE lpSequence)
{
	int i, j, Tempos;
	LPBRELS_TEMPO lpNew;
	BRELS_TEMPO temp;

	Tempos = 0;
	lpSequence->dwTempos = 0;
	lpSequence->lpTempos = NULL;
	lpNew = (LPBRELS_TEMPO) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BRELS_TEMPO)*lpSequence->dwEvents);
	if (lpNew==NULL) return;

	for (i=0; i<lpSequence->wTracks; i++)
	for (j=0; j<(int) lpSequence->Tracks[i].dwEvents; j++)
	if (lpSequence->Tracks[i].Events[j].Event == 0x51)
	{
		lpNew[Tempos].wTrack = i;
		lpNew[Tempos++].dwEvent = j;
	}

	/*if (!IsBadCodePtr((FARPROC) lpSequence->lpTempos))*/
		HeapFree(GetProcessHeap(), 0, lpSequence->lpTempos);
	lpSequence->dwTempos = Tempos;
	lpSequence->lpTempos = (LPBRELS_TEMPO) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BRELS_TEMPO)*Tempos);
	CopyMemory(lpSequence->lpTempos, lpNew, Tempos*sizeof(BRELS_TEMPO));

	for (i=0; i<(Tempos-1); i++)
	for (j=i+1; j<Tempos; j++)
	if (lpSequence->Tracks[lpNew[i].wTrack].Events[lpNew[i].dwEvent].dwTicks>lpSequence->Tracks[lpNew[j].wTrack].Events[lpNew[j].dwEvent].dwTicks)
	{
		temp = lpSequence->lpTempos[i];
		lpSequence->lpTempos[i] = lpSequence->lpTempos[j];
		lpSequence->lpTempos[j] = temp;
	}
	else
	if ((lpNew[i].dwEvent > lpNew[j].dwEvent) && (lpSequence->Tracks[lpNew[i].wTrack].Events[lpNew[i].dwEvent].dwTicks==lpSequence->Tracks[lpNew[j].wTrack].Events[lpNew[j].dwEvent].dwTicks))
	{
		temp = lpSequence->lpTempos[i];
		lpSequence->lpTempos[i] = lpSequence->lpTempos[j];
		lpSequence->lpTempos[j] = temp;
	}


	HeapFree(GetProcessHeap(), 0, lpNew);

}

DWORD GetTrackSize(LPBRELS_MIDI_SEQUENCE lpSequence, WORD wTrack)
{
	BYTE Byte;
	DWORD Offset, Delta, Ticks;
	int i, j, RunningStatus;
	LPBRELS_MIDI_EVENT lpEvent;

	i = wTrack;
	RunningStatus = -1;
	Ticks = 0;
	Offset = 0;

	for (j=0; j<(int) lpSequence->Tracks[i].dwEvents; j++)
	{
		lpEvent = &lpSequence->Tracks[i].Events[j];
		Delta = lpEvent->dwTicks - Ticks;
		Ticks = lpEvent->dwTicks;
		Byte = ((Delta >> 21) & 0x7F);
		if (Byte) Offset++;
		Byte = ((Delta >> 14) & 0x7F);
		if (Byte) Offset++;
		Byte = ((Delta >>  7) & 0x7F);
		if (Byte) Offset++;
		Offset++;

		if ((lpEvent->Event>=0x80) && (lpEvent->Event<=0xEF))
		{
			if (lpEvent->Event!=RunningStatus)
			{
				Offset++;
				RunningStatus = lpEvent->Event;
			}
			Offset+=lpEvent->DataSize;
		}
		else
		if (lpEvent->Event < 0x80)
		{
			Offset+=3;
			Offset+=lpEvent->DataSize;
			RunningStatus = lpEvent->Event;
		}
		else
		{
			Offset++;
			Offset+=lpEvent->DataSize;
			RunningStatus = lpEvent->Event;
		}
	}

	return Offset;
}

void GetTrackData(LPBRELS_MIDI_SEQUENCE lpSequence, WORD wTrack, LPBYTE* lpData)
{
	LPBYTE lpTemp;
	BYTE Byte;
	DWORD Offset, Delta, Ticks;
	int i, j, RunningStatus;
	LPBRELS_MIDI_EVENT lpEvent;

	lpTemp = (LPBYTE) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, GetTrackSize(lpSequence, wTrack));

	i = wTrack;
	RunningStatus = -1;
	Ticks = 0;
	Offset = 0;

	for (j=0; j<(int) lpSequence->Tracks[i].dwEvents; j++)
	{
		lpEvent = &lpSequence->Tracks[i].Events[j];
		Delta = lpEvent->dwTicks - Ticks;
		Ticks = lpEvent->dwTicks;
		Byte = ((Delta >> 21) & 0x7F);
		if (Byte) lpTemp[Offset++] = Byte | 0x80;
		Byte = ((Delta >> 14) & 0x7F);
		if (Byte) lpTemp[Offset++] = Byte | 0x80;
		Byte = ((Delta >>  7) & 0x7F);
		if (Byte) lpTemp[Offset++] = Byte | 0x80;
		Byte = ((Delta      ) & 0x7F);
		lpTemp[Offset++] = Byte;

		if ((lpEvent->Event>=0x80) && (lpEvent->Event<=0xEF))
		{
			if (lpEvent->Event!=RunningStatus)
			{
				lpTemp[Offset++] = lpEvent->Event;
				RunningStatus = lpEvent->Event;
			}
			if (lpEvent->DataSize>0) lpTemp[Offset++] = lpEvent->Data.a[0];
			if (lpEvent->DataSize>1) lpTemp[Offset++] = lpEvent->Data.a[1];
			if (lpEvent->DataSize>2) lpTemp[Offset++] = lpEvent->Data.a[2];
			if (lpEvent->DataSize>3) lpTemp[Offset++] = lpEvent->Data.a[3];
		}
		else
		if (lpEvent->Event < 0x80)
		{
			lpTemp[Offset++] = 0xFF;
			lpTemp[Offset++] = lpEvent->Event;
			lpTemp[Offset++] = lpEvent->DataSize;
			CopyMemory(&lpTemp[Offset], lpEvent->Data.p, lpEvent->DataSize);
			Offset+=lpEvent->DataSize;
			RunningStatus = -1;
		}
		else
		if (lpEvent->Event == 0xF0)
		{
			lpTemp[Offset++] = lpEvent->Event;
			CopyMemory(&lpTemp[Offset], lpEvent->Data.p, lpEvent->DataSize);
			Offset+=lpEvent->DataSize;
		}
		else
		{
			lpTemp[Offset++] = lpEvent->Event;
			if (lpEvent->DataSize>0) lpTemp[Offset++] = lpEvent->Data.a[0];
			if (lpEvent->DataSize>1) lpTemp[Offset++] = lpEvent->Data.a[1];
			if (lpEvent->DataSize>2) lpTemp[Offset++] = lpEvent->Data.a[2];
			if (lpEvent->DataSize>3) lpTemp[Offset++] = lpEvent->Data.a[3];
		}
	}

	*lpData = lpTemp;
}

void AdjustPosition(LPBRELS_MIDI_SEQUENCE lpSequence, WORD wTrack)
{
	int i, j, Event;
	int Controllers[16][256], Programs[16], PitchWheel[16];

	for (i=0; i<16; i++)
	{
		Programs[i]=-1;
		PitchWheel[i]=-1;
		for (j=0; j<256; j++) Controllers[i][j]=-1;
	}

	for (j=0; j<(int) lpSequence->Tracks[wTrack].dwEvents; j++)
	if ((lpSequence->Tracks[wTrack].Events[j].dwTicks<lpSequence->CurrentTicks))
	{
		/* O que já foi */
		Event = lpSequence->Tracks[wTrack].Events[j].Event;
		switch (Event & 0xF0)
		{
		case 0xB0: Controllers[Event & 0x0F][lpSequence->Tracks[wTrack].Events[j].Data.a[0]] = lpSequence->Tracks[wTrack].Events[j].Data.a[1]; break;
		case 0xC0: Programs[Event & 0x0F] = lpSequence->Tracks[wTrack].Events[j].Data.a[0]; break;
		case 0xE0: PitchWheel[Event & 0x0F] = (lpSequence->Tracks[wTrack].Events[j].Data.a[0] << 8) | lpSequence->Tracks[wTrack].Events[j].Data.a[1]; break;
		default: break;
		}
		lpSequence->Tracks[wTrack].dwCurrentEvent = j;
	}
	else
	{
		/* O que falta ir */
		lpSequence->Tracks[wTrack].dwCurrentEvent = j;
		break;
	}

	for (i=0; i<16; i++)
	{
		if (Programs[i]!=-1)
			midiOutShortMsg(lpSequence->Handle, (0xC0 | i) | (Programs[i] << 8));
		if (PitchWheel[i]!=-1) midiOutShortMsg(lpSequence->Handle, (0xE0 | i) | (btlw(PitchWheel[i]) << 8));
		for (j=0; j<256; j++)
			if (Controllers[i][j]!=-1)
				midiOutShortMsg(lpSequence->Handle, (0xB0 | i) | (j << 8) | (Controllers[i][j] << 16));
	}

}

void CALLBACK PlayProc(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	LPBRELS_MIDI_SEQUENCE lpSequence = (LPBRELS_MIDI_SEQUENCE) dwUser;

	UNREFERENCED_PARAMETER(dw1);
	UNREFERENCED_PARAMETER(dw2);
	UNREFERENCED_PARAMETER(uMsg);

	if (IsBadReadPtr((HANDLE) dwUser, sizeof(BRELS_MIDI_SEQUENCE)))
	{
		timeKillEvent(uID);
		return;
	}

	if (lpSequence->dwStatus==MIDI_PLAY)
	{
		if (!lpSequence->Suspended)
		{
			MidiNext((HANDLE) dwUser);
			if (lpSequence->dwStatus==MIDI_PLAY)
				PlayCallback(lpSequence, FALSE);
		}
	}
	else
	{
		timeKillEvent(uID);
		PlayCallback(lpSequence, TRUE);
	}
}

void PlayCallback(LPBRELS_MIDI_SEQUENCE lpSequence, BOOL Ignore)
{
	/* Callback */
	if (IsWindow(lpSequence->Callback))
		if (lpSequence->Processed || Ignore)
		{
			lpSequence->Processed = FALSE;
			PostMessage(lpSequence->Callback, lpSequence->CallbackMessage, (WPARAM) &lpSequence->Processed, (LPARAM) lpSequence);
		}
}
