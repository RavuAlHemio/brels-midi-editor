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

#include <windows.h>
#include <richedit.h>
#include <stdio.h>
#include "brlsmidi.h"
#include "mididec.h"


HWND decwindow, decedit1;
MSG decmsg;
WNDCLASS decwc;
char decHex[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
char decstr1[4000000]; char* lpdecstr1 = (char*) &decstr1;
char decstr2[MAX_PATH]; char* lpdecstr2 = (char*) &decstr2;

LRESULT CALLBACK DecoderDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
		SendMessage(hwnd, WM_COMMAND, IDCANCEL, 0);
		return 0;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			SetWindowLongPtr(hwnd, GWLP_USERDATA, IDOK);
			EnableWindow(GetParent(hwnd), TRUE);
			ShowWindow(hwnd, SW_HIDE);
			break;
		case IDCANCEL:
			SetWindowLongPtr(hwnd, GWLP_USERDATA, IDCANCEL);
			EnableWindow(GetParent(hwnd), TRUE);
			ShowWindow(hwnd, SW_HIDE);
			break;
		default:
			break;
		}
		return 0;
	case WM_SIZE:
		SetWindowPos(decedit1, NULL, 0, 0, LOWORD(lParam), HIWORD(lParam), SWP_NOZORDER);
		return 0;
	case WM_DESTROY:
		UnregisterClass("DECODERDIALOGCLASS", decwc.hInstance);
		return 0;
	default:
		break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int DecoderDialog(HWND hwndParent, HANDLE Sequence)
{
	HANDLE decSequence;
	DWORD nEvents;
	WORD wTracks;
	BOOL Current = Sequence != NULL;
	DWORD i, j, k, Delta, Return;
	SIZE_T Position;
	LPBRELS_MIDI_EVENT lpBuffer;
	OPENFILENAME ofn;
	int maxDataSize;
	char* maxDataSizePadding;

	if (!LoadLibrary("riched20.dll"))
	{
		MessageBox(NULL, "Your system does not support Rich Edit version 2 (that's a serious problem, check it out).", "Error", MB_ICONERROR);
		return 0;
	}

	decwc.style = CS_OWNDC | CS_DBLCLKS;
	decwc.lpfnWndProc = DecoderDialogProc;
	decwc.hInstance = (HINSTANCE) GetClassLongPtr(hwndParent, GCLP_HMODULE);
	decwc.hIcon = (HICON) LoadImage(NULL, "logo.ico", IMAGE_ICON, 16,16,LR_LOADFROMFILE);
	decwc.hCursor = LoadCursor(NULL, IDC_ARROW);
	decwc.hbrBackground = (HBRUSH) (COLOR_3DFACE + 1);
	decwc.lpszMenuName = NULL;
	decwc.lpszClassName = "DECODERDIALOGCLASS";
	RegisterClass(&decwc);

	if (!Current)
	{
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.hInstance = decwc.hInstance;
		ofn.lpstrFilter = "MIDI Files\0*.mid\0All files\0*.*\0";
		ofn.lpstrFile = lpdecstr2;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrDefExt = "MID";
		ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
		ofn.lStructSize = sizeof(OPENFILENAME);

		if (!GetOpenFileName(&ofn)) return 0;
	}

	decwindow = CreateWindow("DECODERDIALOGCLASS", "MIDI Decoder", WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU, GetSystemMetrics(SM_CXSCREEN) / 2 - 640 / 2, GetSystemMetrics(SM_CYSCREEN) / 2 - 480 / 2,640,480,hwndParent, NULL, decwc.hInstance, NULL);
	decedit1 = CreateWindowEx(WS_EX_CLIENTEDGE, RICHEDIT_CLASS, "", ES_WANTRETURN | ES_MULTILINE | WS_HSCROLL | WS_VSCROLL | WS_VISIBLE | WS_CHILD,0,0,640,480,decwindow, NULL, decwc.hInstance,NULL);
	SendMessage(decedit1, EM_SETTEXTMODE, TM_PLAINTEXT, 0);
	SendMessage(decedit1, WM_SETFONT, (WPARAM) GetStockObject(ANSI_FIXED_FONT), 0);

	if (Current)
	{
		decSequence = Sequence;
	}
	else
		if (!MidiOpen(ofn.lpstrFile, (DWORD) -1, &decSequence))
		{
			MessageBox(decwindow, "MidiOpen failed. Close all MIDI applications and try again", "Error", MB_ICONERROR);
			return 0;
		}


	wTracks = (WORD) MidiGet(decSequence, TRACK_COUNT);

	strcpy_s(lpdecstr1, MAX_PATH, "'\xFF' replaces byte 0x00.\r\n\r\n");
	Position = strlen(lpdecstr1);
	for (i=0; i<wTracks; i++)
	{
		sprintf_s(lpdecstr2, MAX_PATH, "%.4g%s", i * 100.00l / wTracks, "%");
		SetWindowText(decwindow, lpdecstr2);

		Delta = 0;
		nEvents = (DWORD) MidiTrackGet(decSequence, (WORD) i, EVENT_COUNT);
		MidiGetTrackEvents(decSequence, (WORD) i, 0, nEvents-1, MIDI_INDEX, &lpBuffer);

		Position+=sprintf_s(&lpdecstr1[Position], MAX_PATH-Position, "Track %d:\r\n", i);

		maxDataSize = 2;
		for (j=0; j<nEvents; j++)
			if ((lpBuffer[j].Event < 0x80) || (lpBuffer[j].Event==0xF0))
				if (lpBuffer[j].DataSize > maxDataSize) maxDataSize = lpBuffer[j].DataSize;
		maxDataSizePadding = (char*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, maxDataSize + 1);
		memset(maxDataSizePadding, ' ', maxDataSize);


		for (j=0; j<nEvents; j++)
		{
			Position+=sprintf_s(
				&lpdecstr1[Position],
				MAX_PATH-Position,
				"%d:\t%x\t",
				lpBuffer[j].dwTicks /*- Delta */,
				lpBuffer[j].Event
			);

			Delta = lpBuffer[j].dwTicks;

			if (((lpBuffer[j].Event < 0x80) && (lpBuffer[j].Event != 0x51)) || (lpBuffer[j].Event==0xF0))
			{
				CopyMemory(&lpdecstr1[Position], lpBuffer[j].Data.p, lpBuffer[j].DataSize);
				Position+=lpBuffer[j].DataSize;
				strncpy_s(&lpdecstr1[Position], MAX_PATH-Position, &maxDataSizePadding[0], maxDataSize-lpBuffer[j].DataSize+2);
				Position+=(maxDataSize-lpBuffer[j].DataSize+2);
				for (k=0; k<lpBuffer[j].DataSize; k++)
				{
					lpdecstr1[Position++] = decHex[lpBuffer[j].Data.p[k] >>   4];
					lpdecstr1[Position++] = decHex[lpBuffer[j].Data.p[k] & 0x0F];
					lpdecstr1[Position++] = ' ';
				}
			}
			else
			{
				/*CopyMemory(&lpdecstr1[Position], lpBuffer[j].Data, lpBuffer[j].DataSize); */
				/*Position+=lpBuffer[j].DataSize; */
				/*strncpy(&lpdecstr1[Position], &maxDataSizePadding[0], maxDataSize-lpBuffer[j].DataSize+2); */
				/*Position+=(maxDataSize-lpBuffer[j].DataSize+2); */
				for (k=0; k<lpBuffer[j].DataSize; k++)
				{
					lpdecstr1[Position++] = decHex[lpBuffer[j].Data.a[k] >>   4];
					lpdecstr1[Position++] = decHex[lpBuffer[j].Data.a[k] & 0x0F];
					lpdecstr1[Position++] = ' ';
				}

				/*
				if (lpBuffer[j].DataSize>=1) 
				{
					strcpy(&lpdecstr1[Position], itoa(lpBuffer[j].Data[0], lpdecstr2, 16));
					Position+=strlen(lpdecstr2);
				}
				strcpy(&lpdecstr1[Position], "\t");
				Position++;

				if (lpBuffer[j].DataSize>=2) 
				{
					strcpy(&lpdecstr1[Position], itoa(lpBuffer[j].Data[1], lpdecstr2, 16));
					Position+=strlen(lpdecstr2);
				}
				strcpy(&lpdecstr1[Position], "\t");
				Position++;

				if (lpBuffer[j].DataSize>=3) 
				{
					strcpy(&lpdecstr1[Position], itoa(lpBuffer[j].Data[2], lpdecstr2, 16));
					Position+=strlen(lpdecstr2);
				}
				strcpy(&lpdecstr1[Position], "\t");
				Position++;

				if (lpBuffer[j].DataSize>=4)
				{
					strcpy(&lpdecstr1[Position], itoa(lpBuffer[j].Data[3], lpdecstr2, 16));
					Position+=strlen(lpdecstr2);
				}
				strcpy(&lpdecstr1[Position], "\t");
				Position++;
				*/
			}
			strcpy_s(&lpdecstr1[Position], MAX_PATH-Position, "\r\n");
			Position+=2;
		}
		
		HeapFree(GetProcessHeap(), 0, maxDataSizePadding);
		strcpy_s(&lpdecstr1[Position], MAX_PATH-Position, "\r\n\r\n");
		Position+=4;
		MidiCleanEvents(nEvents, lpBuffer);
	}

	/*Remove line-breaking characters */
	for (i=1; i<Position; i++)
	{
		if (lpdecstr1[i]==0) lpdecstr1[i]='\xFF';
		if (  (lpdecstr1[i]==11) ||
			(lpdecstr1[i]==12) ||
			((lpdecstr1[i]=='\n') && (lpdecstr1[i-1]!='\r')) ||
			((lpdecstr1[i]=='\r') && (lpdecstr1[i+1]!='\n')))
				lpdecstr1[i]=' ';
	}

	lpdecstr1[Position]=0;
	

	SetWindowText(decwindow, "MIDI Decoder");
	SetWindowText(decedit1, lpdecstr1);

	EnableWindow(hwndParent, FALSE);

	while (IsWindowVisible(decwindow))
	if (PeekMessage(&decmsg, NULL, 0, 0, PM_REMOVE))
	if (!IsDialogMessage(GetActiveWindow(), &decmsg))
	{
		TranslateMessage(&decmsg);
		DispatchMessage(&decmsg);
	}

	Return = (DWORD) GetWindowLongPtr(decwindow, GWLP_USERDATA);

	if (!Current)
		MidiClose(decSequence);

	DestroyWindow(decwindow);

	EnableWindow(hwndParent, TRUE);

	return Return;
}
