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

#define _WIN32_IE 0x0800

#include <math.h>
#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>
#include "brlsmidi.h"
#include "inputbox.h"
#include "mididec.h"

#define ID_FILENEW            0
#define ID_FILEOPEN           1
#define ID_FILESAVE           2
#define ID_FILESAVEAS         3
#define ID_PLAY               4
#define ID_HOME               5
#define ID_PAUSE              6
#define ID_END                7
#define ID_STOP               8
#define ID_MUTE               9
#define ID_SOLO               10
#define ID_LOOP               11
#define ID_INSERTTRACK        12
#define ID_REMOVETRACK        13
#define ID_COPYRIGHT          14
#define ID_TRACKNAME          15
#define ID_TRACKNUMBER        16
#define ID_INSTRUMENTNAME     17
#define ID_HELP               18
#define ID_ABOUT              19
#define ID_API                20
#define ID_CUT                21
#define ID_COPY               22
#define ID_PASTE              23
#define ID_DELETE             24
#define ID_SELECTALL          25
#define ID_TRANSPOSEUP        26
#define ID_TRANSPOSEDOWN      27
#define ID_OCTAVEUP           28
#define ID_OCTAVEDOWN         29
#define ID_SELECT             30
#define ID_0                  31
#define ID_1                  32
#define ID_2                  33
#define ID_4                  34
#define ID_8                  35
#define ID_16                 36
#define ID_32                 37
#define ID_64                 38
#define ID_PROGRAM            39
#define ID_TEMPO              40
#define ID_TEXT               41
#define ID_LYRIC              42
#define ID_MARKER             43
#define ID_CUEPOINT           44
#define ID_UNKNOWN            45
#define ID_FILTER             46
#define ID_DECODE             47

#define ID_TRACK              48
#define ID_DEVICE             49
#define ID_ZOOM               50
#define ID_BEAT               51
#define ID_CONTROLLER         52
#define ID_NOTE               53
#define ID_POSITION           54

#define BeatWidth             96.0l
#define KeyBaseHeight         11.0l

#define META_INSERT           100
#define META_DELETE           101
#define META_UPDATE           102
#define META_SELECT           103

#define RULER                 1
#define BAR                   2
#define PIANO                 3
#define NOTES                 4
#define CONTROLLERS           5

#define ARROW                 0
#define HAND                  1
#define UPARROW               2
#define SIZEALL               3
#define SIZEWE                4

#define SELECTING             1
#define RANGING               2
#define RESIZING              3
#define TRANSPOSING           4
#define PLACING               5
#define DRAGGING              6

#define WM_CALLBACK           WM_APP + 1
#define WM_REDRAW             WM_APP + 2

#define LANGID_PT_BR          0x0416
#define LANGID_PT_PT          0x0816

#define STRINGS               88

HWND window, tooltip, splash, tutorial;
MSG msg;
WNDCLASSEX wc;
INITCOMMONCONTROLSEX iccex;
HIMAGELIST imagelist;
HDC hdc, buffer;
HBITMAP bitmap;
HFONT hfont;
HBRUSH red, blue, black, white, hollow;
HPEN blackp, whitep, redp, greenp, dottedp, nullp, gray1, gray2, gray3, gray4, gray5;
RECT Piano, Ruler, Bar, Controllers, Notes;
HCURSOR Cursors[5];

char str1[MAX_PATH], str2[MAX_PATH], str3[MAX_PATH], filename[MAX_PATH];
char* lpstr1 = (char*) &str1;
char* lpstr2 = (char*) &str2;
char* lpstr3 = (char*) &str3;
char* lpfile = (char*) &filename;
char Instruments[11][128][256];
char Strings[STRINGS][1024];

BYTE trackSignature[2] = {0x00, 0x00};
BYTE defaultTempo[3] = {0x07, 0xA1, 0x20};

HANDLE Sequence;
DWORD Device;
BOOL Saved, PlayButtonsAdjusted, Positioning, RButton, LButton, FirstFileOpen;
BOOL PlayDone, InController;
RECT Selection;
int CurrentNote, CurrentPosition;
int FirstNote, LastNote, PlayedNote, OffsetX, OffsetY, MouseX, MouseY;
int PageSize, CursorSet;
int Beat, Track, Controller, Mode, Meta, Movement, ControllerValue;
int nClipboard, ClipboardBeat;
LPBRELS_MIDI_EVENT Clipboard;
double TickWidth, KeyHeight, Zoom;

char* trim(char* src);
void KeyText(int Key, LPSTR buffer);
void LoadText(const char* File, char* Structure, int Items, int Item_Size);
int NoteVK(WPARAM wParam);
void GetInstrumentName(int BankHigh, int BankLow, int Program, LPSTR lpBuffer);
void GetProgramInformation(int nTrack, int Position, int* Channel, int* BankHigh, int* BankLow, int* Program);
BOOL GetChannelProgramInformation(int Channel, int Position, int* BankHigh, int* BankLow, int* Program);
void RenumberTracks(void);
void SplitChannels(void);
BOOL MetaAction(int Action, int Track, int Position, QWORD qwFilter, LPSTR lpData, int DataSize);
void ResizeTrack(WORD wTrack, WORD NewBeat, WORD OldBeat);
void Transpose(int X, int Y);
void ResizeNote(void);
void UpdateController(int Y);
void DeleteController(void);
void PlaceNote(int Length);
void PlaceMeta(void);
void RemoveMeta(void);
LRESULT CALLBACK ProgramProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void ChannelList(HWND hwnd, int Channel);
void ProgramList(HWND hwnd, int Bank);
void PlaceProgram(void);
LRESULT CALLBACK TempoProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void RemoveProgram(void);
void PlaceTempo(void);
void CopyEvents(void);
void DeleteEvents(void);
void PasteEvents(void);
void SelectAllEvents(void);
int SelectEvents(void);
void ChangeDevice(void);
void ChangeBeat(void);
void InsertTrack(void);
void RemoveTrack(void);
void Callback(void);
BOOL FileNew(void);
BOOL CheckSave(LPSTR Title);
BOOL FileOpen(void);
BOOL FileSave(LPSTR lpstrFile);
void WindowSize(WPARAM wParam, LPARAM lParam);
void WindowPaint(WPARAM wParam, LPARAM lParam);
void WindowHScroll(WPARAM wParam, LPARAM lParam);
void WindowVScroll(WPARAM wParam, LPARAM lParam);
void WindowMouseMove(WPARAM wParam, LPARAM lParam);
void WindowLButtonUp(WPARAM wParam, LPARAM lParam);
void WindowLButtonDown(WPARAM wParam, LPARAM lParam);
void WindowRButtonUp(WPARAM wParam, LPARAM lParam);
void WindowRButtonDown(WPARAM wParam, LPARAM lParam);
void KeyUp(WPARAM wParam, LPARAM lParam);
void KeyDown(WPARAM wParam, LPARAM lParam);
void PlayPiano(int Note);
void SilentPiano(void);
void CalculateMeasures(void);
int  GetArea(WPARAM wParam, LPARAM lParam);
void ChooseCursor(int Area);
void CheckCursor(void);
void Redraw(void);
void DoRedraw(void);
void AdjustWindow(void);
void AdjustButtons(void);
void AdjustTrackChannels(WORD wTrack);
void AdjustScroll(void);
void AdjustTitleBar(void);
void FlatButton(int ID, BOOL Flat);
BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam);
LRESULT CALLBACK GroupBoxProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void FilterSequence(void);
void FilterTrack(HANDLE NewHandle, WORD wSrcTrack, WORD wDstTrack, QWORD qwBase, QWORD qwStart, QWORD qwEnd, QWORD qwFilter, BOOL RoundTicks);
BOOL DoFilterSequence(HWND hwnd);
LRESULT CALLBACK FilterProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ControllerProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void GetProgramInformationFromEvent(LPBRELS_MIDI_EVENT lpEvent, int* Channel, int* BankHigh, int* BankLow, int* Program);



char* trim(char* src)
{
	int start=0, end=strlen(src)-1;

	if (end<1) return src;

	while ((src[start]==' ') || (src[end]==' '))
	{
		if (src[start]==' ') start++;
		if (src[end]==' ') end--;
	}

	if (end-start)
		strncpy(src, &src[start], end-start);

	return src;
}

double round(double value)
{
	double fl, cl;
	if ((value - (fl=floor(value))) > ((cl=ceil(value))-value))
		return cl;
	else
		return fl;
}

void KeyText(int Key, LPSTR lpBuffer)
{
	char temp[256];
	int Octave;

	lpBuffer[0]=0;
	Octave = Key / 12;
	if (Octave<10) strcat(lpBuffer, "0");
	strcat(lpBuffer, itoa(Octave, (char*) &temp, 10));
	strcat(lpBuffer, "-");

	switch (Key % 12)
	{
	case  0: strcat(lpBuffer, "C" ); break;
	case  1: strcat(lpBuffer, "C#"); break;
	case  2: strcat(lpBuffer, "D" ); break;
	case  3: strcat(lpBuffer, "D#"); break;
	case  4: strcat(lpBuffer, "E" ); break;
	case  5: strcat(lpBuffer, "F" ); break;
	case  6: strcat(lpBuffer, "F#"); break;
	case  7: strcat(lpBuffer, "G" ); break;
	case  8: strcat(lpBuffer, "G#"); break;
	case  9: strcat(lpBuffer, "A" ); break;
	case 10: strcat(lpBuffer, "A#"); break;
	case 11: strcat(lpBuffer, "B" ); break;
	default: break;
	}
}

void LoadText(const char* File, char* Structure, int Items, int Item_Size)
{
	int i, LastLine, Lines;
	DWORD dwSize, dwOutput;
	char temp[65536];
	HANDLE hFile;

	ZeroMemory(Structure, Items * Item_Size);
	ZeroMemory(&temp, 65536);

	hFile = CreateFile(File, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE) return;
	dwSize = GetFileSize(hFile, NULL);
	ReadFile(hFile, &temp, dwSize, &dwOutput, NULL);
	CloseHandle(hFile);

	LastLine = 0;
	Lines = 0;
	for (i=0; i<(int) dwSize; i++)
	{
		if (temp[i]=='\n')
		{
			strncpy(&Structure[Item_Size*Lines++], &temp[LastLine], i-LastLine-1);
			LastLine = i+1;
		}
	}

	for (i=0; i<(Items*Item_Size-1); i++)
		switch (Structure[i])
		{
		case '~': Structure[i]='\r';break;
		case '^': Structure[i]='\n'; break;
		case '$': Structure[i]='\0'; break;
		default: break;
		}
}

int NoteVK(WPARAM wParam)
{
	switch (wParam)
	{
	case 'A': return  0;
	case 'W': return  1;
	case 'S': return  2;
	case 'E': return  3;
	case 'D': return  4;
	case 'F': return  5;
	case 'T': return  6;
	case 'G': return  7;
	case 'Y': return  8;
	case 'H': return  9;
	case 'U': return 10;
	case 'J': return 11;
	case 'K': return 12;
	case 'O': return 13;
	case 'L': return 14;
	case 'P': return 15;
	default: break;
	}

	return -1;
}

void GetInstrumentName(int BankHigh, int BankLow, int Program, LPSTR lpBuffer)
{
	strcpy(lpBuffer, "");
	switch (BankHigh)
	{
	case   -1: if (Program!=-1) strcpy(lpBuffer, (char*) &Instruments[0][Program]); break;
	case 0x78: if (Program!=-1) strcpy(lpBuffer, (char*) &Instruments[10][Program]); break;
	case 0x79: if ((Program!=-1) && (BankLow!=-1) && (BankLow<10)) strcpy(lpBuffer, (char*) &Instruments[BankLow][Program]); break;
	default  : if ((Program!=-1) && (BankHigh<10)) strcpy(lpBuffer, (char*) &Instruments[BankHigh][Program]); break;
	}
}

void GetProgramInformationFromEvent(LPBRELS_MIDI_EVENT lpEvent, int* Channel, int* BankHigh, int* BankLow, int* Program)
{
	BYTE Event = lpEvent->Event;
	if ((Event>=0x80) && (Event<=0xEF))
	{
		if (Channel!=NULL) *Channel = Event & 0x0F;
		if ((Event & 0xF0)==0xB0)
		{
			if ((lpEvent->Data.a[0]==0x00) && (BankHigh!=NULL))
				*BankHigh = lpEvent->Data.a[1];
			if ((lpEvent->Data.a[0]==0x20) && (BankLow!=NULL))
				*BankLow = lpEvent->Data.a[1];
		}
		if ((Event & 0xF0)==0xC0)
			if (Program!=NULL) *Program = lpEvent->Data.a[0];
	}

}

void GetProgramInformation(int nTrack, int Position, int* Channel, int* BankHigh, int* BankLow, int* Program)
{
	int i, nEvents;
	LPBRELS_MIDI_EVENT lpEvents;
	BOOL ProgramChannel = FALSE;

	nEvents = MidiTrackGet(Sequence, nTrack, EVENT_COUNT);
	MidiGetTrackEvents(Sequence, nTrack, 0, nEvents-1, MIDI_INDEX, &lpEvents);

	if (Channel !=NULL) *Channel  = -1;
	if (Program !=NULL) *Program  = -1;
	if (BankHigh!=NULL) *BankHigh = -1;
	if (BankLow !=NULL) *BankLow  = -1;

	for (i=0; i<nEvents; i++)
	{
		if ((Position==-1) || (((int) lpEvents[i].dwTicks) <= Position))
		{
			int tempChannel = -1;
			GetProgramInformationFromEvent(&lpEvents[i], &tempChannel, BankHigh, BankLow, Program);
			if ((lpEvents[i].Event & 0xF0)==0xC0)
			{
				ProgramChannel = TRUE;
				if (Channel!=NULL) *Channel = tempChannel;
			}
			else
				if (!ProgramChannel)
					if (Channel!=NULL) *Channel = tempChannel;
		}
	}

	MidiCleanEvents(nEvents, lpEvents);
}

BOOL GetChannelProgramInformation(int Channel, int Position, int* BankHigh, int* BankLow, int* Program)
{
	int i, j, nEvents, Return;
	int tempChannel=-1, tempBankHigh=-1, tempBankLow=-1, tempProgram=-1, type;
	LPBRELS_MIDI_EVENT lpEvents;

	Return = FALSE;
	if (Program !=NULL) *Program  = -1;
	if (BankHigh!=NULL) *BankHigh = -1;
	if (BankLow !=NULL) *BankLow  = -1;

	for (j=0; j<(int) MidiGet(Sequence, TRACK_COUNT); j++)
	{

		nEvents = MidiTrackGet(Sequence, j, EVENT_COUNT);
		MidiGetTrackEvents(Sequence, j, 0, nEvents-1, MIDI_INDEX, &lpEvents);

		for (i=0; i<nEvents; i++)
		{
			if ((Position==-1) || (((int) lpEvents[i].dwTicks) <= Position))
			{
				type = (lpEvents[i].Event & 0xF0);
				if ((type==0xC0) || (type==0xB0))
				{
					GetProgramInformationFromEvent(&lpEvents[i], &tempChannel, &tempBankHigh, &tempBankLow, &tempProgram);
					if (Channel == tempChannel)
					{
						if (Program !=NULL) if (type==0xC0) *Program  = tempProgram;
						if (BankHigh!=NULL) *BankHigh = tempBankHigh;
						if (BankLow !=NULL) *BankLow  = tempBankLow;
						Return = TRUE;
					}
				}
			}
		}

		MidiCleanEvents(nEvents, lpEvents);
	}
	return Return;
}

void RenumberTracks(void)
{
	int i;
	for (i=0; i<(int) MidiGet(Sequence, TRACK_COUNT); i++)
	{
		lpstr1[0]=i >> 8;
		lpstr1[1]=i & 0xFF;
		MetaAction(META_UPDATE, i, -1, FILTER_SEQUENCENUMBER, lpstr1, 2);
	}
}

void SplitChannels(void)
{
	int i, nEvents, nLeft, Channel, Tracks, New;
	LPBRELS_MIDI_EVENT lpEvents, lpNew;

	Channel = -1;
	Tracks = 0;
	nEvents = MidiTrackGet(Sequence, 0, EVENT_COUNT);
	nLeft = nEvents;
	MidiGetTrackEvents(Sequence, 0, 0, nEvents-1, MIDI_INDEX, &lpEvents);
	MidiRemoveTrack(Sequence, 0);
	lpNew = (LPBRELS_MIDI_EVENT) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nEvents*sizeof(BRELS_MIDI_EVENT));
	while (nLeft)
	{
		New = 0;
		for (i=0; i<nEvents; i++)
		if (lpEvents[i].Event!=0xFF)
		{
			if ((lpEvents[i].Event>=0x80) && (lpEvents[i].Event<=0xEF))
			{
				if ((lpEvents[i].Event & 0x0F)==Channel)
				{
					lpNew[New++] = lpEvents[i];
					lpEvents[i].Event=0xFF;
				}
			}
			else
			{
				lpNew[New++] = lpEvents[i];
				lpEvents[i].Event=0xFF;
			}
		}
		if (New)
		{
			nLeft-=New;
			MidiInsertTrack(Sequence, Tracks);
			MidiInsertTrackEvents(Sequence, Tracks, New, lpNew);
			Tracks++;
		}
		Channel++;
	}
	MidiCleanEvents(nEvents, lpEvents);
	MidiFreeBuffer(lpNew);
	Saved = FALSE;
}

BOOL MetaAction(int Action, int nTrack, int Position, QWORD qwFilter, LPSTR lpData, int DataSize)
{

	BOOL Return;
	int nFilters, Start, End;
	LPBRELS_FILTERED_EVENT lpFilters;
	BRELS_MIDI_EVENT Event;

	Return = FALSE;
	Start = Position;
	End = Position;
	Event.wTag = FALSE;
	Event.dwTicks = Position;
	Event.Event = MidiQuery(Sequence, nTrack, MIDI_EVENT, MIDI_FILTER, qwFilter);
	Event.DataSize = DataSize;
	Event.Data.p = (LPBYTE) lpData;

	if (Position==-1)
	{
		Start = 0;
		End = MidiGet(Sequence, TICK_COUNT);
		Event.dwTicks = 0;
	}
	nFilters = MidiFilterTrackEvents(Sequence, nTrack, Start, End, MIDI_TICKS, qwFilter, &lpFilters);

	if (Action==META_SELECT)
	{
		ZeroMemory(lpData, DataSize);
		if (nFilters)
		{
			Event.Data.p = (LPBYTE) (DWORD) MidiEventGet(Sequence, nTrack, lpFilters[0].qwStart, EVENT_DATA);
			CopyMemory(lpData, Event.Data.p, MidiEventGet(Sequence, nTrack, lpFilters[0].qwStart, EVENT_DATASIZE));
			MidiFreeBuffer(Event.Data.p);
			Return = TRUE;
		}
	}

	if ((Action==META_DELETE) || (Action==META_UPDATE))
		if (nFilters)
		{
			MidiRemoveTrackEvents(Sequence, nTrack, lpFilters[0].qwStart, lpFilters[0].qwStart, MIDI_INDEX);
			Return = TRUE;
			Saved = FALSE;
		}

	if ((Action==META_INSERT) || (Action==META_UPDATE))
	{
		MidiInsertTrackEvents(Sequence, nTrack, 1, &Event);
		Return = TRUE;
		Saved = FALSE;
	}

	if (nFilters) MidiFreeBuffer(lpFilters);

	return Return;
}

void ResizeTrack(WORD wTrack, WORD NewBeat, WORD OldBeat)
{
	int i, nEvents, nFilters;
	LPBRELS_MIDI_EVENT lpEvents;
	LPBRELS_FILTERED_EVENT lpFilters;
	double Factor;

	Factor = (double) NewBeat / (double) OldBeat;

	nEvents = MidiTrackGet(Sequence, wTrack, EVENT_COUNT);
	MidiGetTrackEvents(Sequence, wTrack, 0, nEvents-1, MIDI_INDEX, &lpEvents);
	nFilters = MidiFilterTrackEvents(Sequence, wTrack, 0, nEvents-1, MIDI_INDEX, FILTER_NOTES, &lpFilters);

	for (i=0; i<nEvents; i++)
		lpEvents[i].dwTicks = round(lpEvents[i].dwTicks * Factor);

	/* Fixes zero-length notes */
	for (i=0; i<nFilters; i++)
		if (lpEvents[lpFilters[i].qwStart].dwTicks==lpEvents[lpFilters[i].qwEnd].dwTicks)
			lpEvents[lpFilters[i].qwEnd].dwTicks = lpEvents[lpFilters[i].qwStart].dwTicks + 1;

	MidiRemoveTrack(Sequence, wTrack);
	MidiInsertTrack(Sequence, wTrack);
	MidiInsertTrackEvents(Sequence, wTrack, nEvents, lpEvents);
	MidiCleanEvents(nEvents, lpEvents);
	MidiFreeBuffer(lpFilters);
}

void Transpose(int X, int Y)
{
	int i, j, nEvents, nFilters, nProgramChanges, Count, Byte1, Ticks, currentChannelTicks, currentChannel;
	LPBRELS_MIDI_EVENT lpEvents, Event, EventEnd;
	LPBRELS_FILTERED_EVENT lpFilters, lpProgramChanges;

	if (!X && !Y) return;

	nEvents = MidiTrackGet(Sequence, Track, EVENT_COUNT);
	MidiGetTrackEvents(Sequence, Track, 0, nEvents-1, MIDI_INDEX, &lpEvents);
	nFilters = MidiFilterTrackEvents(Sequence, Track, 0, nEvents-1, MIDI_INDEX, FILTER_NOTES, &lpFilters);
	nProgramChanges = MidiFilterTrackEvents(Sequence, Track, 0, nEvents-1, MIDI_INDEX, FILTER_PROGRAM, &lpProgramChanges);
	currentChannelTicks = -1;
	currentChannel = 0;

	Count = 0;
	for (i=0; i<nFilters; i++)
		if (lpEvents[lpFilters[i].qwStart].wTag) Count++;

	for (i=0; i<nFilters; i++)
	if ((!Count) || (lpEvents[lpFilters[i].qwStart].wTag))
	{
		Ticks = X + lpEvents[lpFilters[i].qwStart].dwTicks;
		Byte1 = Y + lpEvents[lpFilters[i].qwStart].Data.a[0];
		if ((Byte1>=0) && (Byte1<=127) && (Ticks>=0))
		{
			lpEvents[lpFilters[i].qwStart].dwTicks = Ticks;
			lpEvents[lpFilters[i].qwStart].Data.a[0] = Byte1;
			Ticks = X + (lpEvents[lpFilters[i].qwEnd].dwTicks);
			Byte1 = Y + lpEvents[lpFilters[i].qwEnd].Data.a[0];
			lpEvents[lpFilters[i].qwEnd].dwTicks = Ticks;
			lpEvents[lpFilters[i].qwEnd].Data.a[0] = Byte1;
		}

		Event = &lpEvents[lpFilters[i].qwStart];
		EventEnd = &lpEvents[lpFilters[i].qwEnd];

		/* Adjusts channeled events */
		if ((Event->Event >= 0x80) && (Event->Event <= 0x9F))
		{
			if (((int) Event->dwTicks) > currentChannelTicks )
			{
				for (j=0; j < nProgramChanges; j++)
					if (lpEvents[lpProgramChanges[j].qwStart].dwTicks <= Event->dwTicks)
					{
						currentChannelTicks = lpEvents[lpProgramChanges[j].qwStart].dwTicks;
						currentChannel = lpEvents[lpProgramChanges[j].qwStart].Event & 0x0F;
					}
					else break;
			}
			Event->Event = (Event->Event & 0xF0) | currentChannel;
			EventEnd->Event = (EventEnd->Event & 0xF0) | currentChannel;
		}
	}

	MidiSuspend(Sequence, TRUE);
	MidiSilence(Sequence);
	MidiRemoveTrack(Sequence, Track);
	MidiInsertTrack(Sequence, Track);
	MidiInsertTrackEvents(Sequence, Track, nEvents, lpEvents);
	MidiResume(Sequence, TRUE);

	MidiCleanEvents(nEvents, lpEvents);
	MidiFreeBuffer(lpFilters);
	MidiFreeBuffer(lpProgramChanges);
	Saved = FALSE;
}

void ResizeNote(void)
{
	int i, nEvents, nFilters, temp;
	LPBRELS_MIDI_EVENT lpEvents;
	LPBRELS_FILTERED_EVENT lpFilters;

	if (Selection.right<0) return;

	nEvents = MidiTrackGet(Sequence, Track, EVENT_COUNT);
	MidiGetTrackEvents(Sequence, Track, 0, nEvents-1, MIDI_INDEX, &lpEvents);
	nFilters = MidiFilterTrackEvents(Sequence, Track, 0, nEvents-1, MIDI_INDEX, FILTER_NOTES, &lpFilters);

	for (i=0; i<nFilters; i++)
	{
		if ((lpEvents[lpFilters[i].qwStart].dwTicks==(DWORD) Selection.left) && (lpEvents[lpFilters[i].qwStart].wTag))
		{
			if ((((DWORD) Selection.right)!=lpEvents[lpFilters[i].qwEnd].dwTicks) /*&& (Selection.bottom==lpEvents[lpFilters[i].qwStart].Data.a[0])*/)
				lpEvents[lpFilters[i].qwStart].dwTicks = Selection.right;
		}
		else
			if ((lpEvents[lpFilters[i].qwEnd].dwTicks==(DWORD) Selection.left) && (lpEvents[lpFilters[i].qwEnd].wTag))
			{
				if ((((DWORD) Selection.right)!=lpEvents[lpFilters[i].qwStart].dwTicks) /*&& (Selection.bottom==lpEvents[lpFilters[i].qwEnd].Data.a[0])*/)
					lpEvents[lpFilters[i].qwEnd].dwTicks = Selection.right;
			}
			else
				continue;
		if (lpEvents[lpFilters[i].qwStart].dwTicks>lpEvents[lpFilters[i].qwEnd].dwTicks)
		{
			temp = lpEvents[lpFilters[i].qwStart].dwTicks;
			lpEvents[lpFilters[i].qwStart].dwTicks = lpEvents[lpFilters[i].qwEnd].dwTicks;
			lpEvents[lpFilters[i].qwEnd].dwTicks = temp;
		}
	}

	MidiSuspend(Sequence, TRUE);
	MidiSilence(Sequence);
	MidiRemoveTrack(Sequence, Track);
	MidiInsertTrack(Sequence, Track);
	MidiInsertTrackEvents(Sequence, Track, nEvents, lpEvents);
	MidiResume(Sequence, TRUE);

	MidiCleanEvents(nEvents, lpEvents);
	MidiFreeBuffer(lpFilters);
}

void UpdateController(int Y)
{
	BOOL Updated;
	int i, nEvents, Value, Channel;
	LPBRELS_MIDI_EVENT lpEvents;
	BRELS_MIDI_EVENT Event;

	Value = 127-(Y-Controllers.top);
	if ((Value<0) || (Value>127)) return;
	if ((GetAsyncKeyState(VK_SHIFT)>>15)!=0) Value = 64;
	InController=TRUE;
	ControllerValue = Value;

	MidiSuspend(Sequence, TRUE);
	MidiSilence(Sequence);
	nEvents = MidiTrackGet(Sequence, Track, EVENT_COUNT);
	MidiGetTrackEvents(Sequence, Track, 0, nEvents-1, MIDI_INDEX, &lpEvents);
	MidiRemoveTrack(Sequence, Track);
	MidiInsertTrack(Sequence, Track);
	Updated = FALSE;
	for (i=0; i<nEvents; i++)
	{
		if (lpEvents[i].dwTicks!=(DWORD) Selection.right) continue;
		Event.Event = lpEvents[i].Event & 0xF0;
		if ((Controller==-2) && (Event.Event==0x90) && (Value>0) && (lpEvents[i].Data.a[1]>0))
		{
			lpEvents[i].Data.a[1] = Value;
			Updated = TRUE;
		}
		if ((Controller==-1) && (Event.Event==0xE0))
		{
			lpEvents[i].Data.a[0] = 0;
			lpEvents[i].Data.a[1] = Value;
			Updated = TRUE;
		}
		if ((Controller>= 0) && (Event.Event==0xB0) && (lpEvents[i].Data.a[0]==Controller))
		{
			lpEvents[i].Data.a[1] = Value;
			Updated = TRUE;
		}
	}
	MidiInsertTrackEvents(Sequence, Track, nEvents, lpEvents);
	if ((!Updated) && (Controller!=-2))
	{
		GetProgramInformation(Track, -1, &Channel, NULL, NULL, NULL);
		if (Channel==-1) Channel=0;
		Event.wTag = FALSE;
		Event.dwTicks = Selection.right;
		Event.DataSize = 2;
		Event.Data.a[1] = Value;
		if (Controller==-1)
		{
			Event.Event = 0xE0 | Channel;
			Event.Data.a[0] = 0;
		}
		else
		{
			Event.Event = 0xB0 | Channel;
			Event.Data.a[0] = Controller;
		}
		MidiInsertTrackEvents(Sequence, Track, 1, &Event);
	}
	Saved = FALSE;
	MidiCleanEvents(nEvents, lpEvents);
	MidiResume(Sequence, TRUE);
	AdjustScroll();
}

void DeleteController(void)
{
	int i, Event;
	LPBYTE lpData;

	if (Controller==-2) return;

	MidiSuspend(Sequence, TRUE);
	MidiSilence(Sequence);
	for (i=0; i<(int) MidiTrackGet(Sequence, Track, EVENT_COUNT); i++)
	{
		if (MidiEventGet(Sequence, Track, i, EVENT_TICKS)!=(DWORD) Selection.right) continue;
		Event = MidiEventGet(Sequence, Track, i, EVENT_EVENT) & 0xF0;
		lpData = (LPBYTE) (DWORD) MidiEventGet(Sequence, Track, i, EVENT_DATA);
		if ((Controller==-1) && (Event==0xE0))
		{
			MidiRemoveTrackEvents(Sequence, Track, i, i, MIDI_INDEX);
			Saved = FALSE;
			i--;
		}
		if ((Controller!=-1) && (Event==0xB0) && (lpData[0]==Controller))
		{
			MidiRemoveTrackEvents(Sequence, Track, i, i, MIDI_INDEX);
			Saved = FALSE;
			i--;
		}
		MidiFreeBuffer(lpData);
	}
	MidiResume(Sequence, TRUE);
	AdjustScroll();
}

void PlaceNote(int Length)
{
	int Channel;
	BRELS_MIDI_EVENT Events[2];

	memset(&Events, 0, sizeof(Events));
	Events[0].Event = 0x90;
	Events[0].DataSize = 2;
	Events[0].dwTicks = Selection.right;
	Events[1].Event = 0x90;
	Events[1].dwTicks = Selection.right + Length;
	Events[1].DataSize = 2;

	MidiSuspend(Sequence, TRUE);
	MidiSilence(Sequence);
	GetProgramInformation(Track, Selection.right, &Channel, NULL, NULL, NULL);
	if (Channel==-1) Channel=0;
	Events[0].Event = 0x90 | Channel;
	Events[0].Data.a[0] = Selection.bottom;
	Events[0].Data.a[1] = 64;
	Events[1].Event = 0x90 | Channel;
	Events[1].Data.a[0] = Selection.bottom;
	MidiInsertTrackEvents(Sequence, Track, 2, (LPBRELS_MIDI_EVENT) &Events);
	MidiResume(Sequence, TRUE);
	AdjustScroll();
	Saved = FALSE;
}

void PlaceMeta(void)
{
	BOOL Update;
	int qwFilter;

	if (Selection.right<0) return;
	if (Selection.left<0) return;

	qwFilter = 0;
	switch (Meta)
	{
	case ID_PROGRAM : PlaceProgram(); break;
	case ID_TEMPO   : PlaceTempo(); break;
	case ID_TEXT    : strcpy(lpstr2, Strings[1]); strcpy(lpstr3, Strings[2]); qwFilter = FILTER_TEXT; break;
	case ID_LYRIC   : strcpy(lpstr2, Strings[3]); strcpy(lpstr3, Strings[4]); qwFilter = FILTER_LYRIC; break;
	case ID_MARKER  : strcpy(lpstr2, Strings[5]); strcpy(lpstr3, Strings[6]); qwFilter = FILTER_MARKER; break;
	case ID_CUEPOINT: strcpy(lpstr2, Strings[7]); strcpy(lpstr3, Strings[8]); qwFilter = FILTER_CUEPOINT; break;
	default: break;
	}

	if (qwFilter)
	{
		Update = MetaAction(META_SELECT, Track, Selection.left, qwFilter, lpstr1, 256);
		if ((!Update) || (Selection.right==Selection.left))
			Update = InputBox(window, lpstr2, lpstr3, lpstr1)==IDOK;
		if (Update)
		{
			MidiSuspend(Sequence, TRUE);
			MidiSilence(Sequence);
			MetaAction(META_DELETE, Track, Selection.left , qwFilter, lpstr1, 256);
			MetaAction(META_INSERT, Track, Selection.right, qwFilter, lpstr1, strlen(lpstr1));
			MidiResume(Sequence, TRUE);
			AdjustScroll();
		}
	}
}

void RemoveMeta(void)
{
	QWORD qwFilter;

	switch (Meta)
	{
	case ID_PROGRAM : qwFilter = FILTER_PROGRAM ; break;
	case ID_TEMPO   : qwFilter = FILTER_TEMPO   ; break;
	case ID_TEXT    : qwFilter = FILTER_TEXT    ; break;
	case ID_LYRIC   : qwFilter = FILTER_LYRIC   ; break;
	case ID_MARKER  : qwFilter = FILTER_MARKER  ; break;
	case ID_CUEPOINT: qwFilter = FILTER_CUEPOINT; break;
	case ID_UNKNOWN : qwFilter = FILTER_CHANNELPREFIX | FILTER_SMPTEOFFSET | FILTER_TIMESIGNATURE | FILTER_KEYSIGNATURE | FILTER_SEQUENCERSPECIFIC | FILTER_SYSTEMEXCLUSIVE | FILTER_KEYAFTERTOUCH | FILTER_CHANNELPRESSURE | FILTER_MTCQUARTERFRAME | FILTER_SONGPOSITIONPOINTER | FILTER_SONGSELECT | FILTER_TUNEREQUEST /*| FILTER_MIDICLOCK | FILTER_MIDISTART | FILTER_MIDICONTINUE | FILTER_MIDISTOP*/ | FILTER_ACTIVESENSE; break;
	default         : return;
	}

	MidiSuspend(Sequence, TRUE);
	MidiSilence(Sequence);
	if (qwFilter==FILTER_PROGRAM)
		RemoveProgram();
	else
		MetaAction(META_DELETE, Track, Selection.left, qwFilter, NULL, 0);
	MidiResume(Sequence, TRUE);
	AdjustWindow();
}

void RemoveProgram(void)
{
	int i, nEvents, Event, Ticks;
	LPBYTE lpData;

	/* Removes every bank select controller related to this program change */
	nEvents = MidiTrackGet(Sequence, Track, EVENT_COUNT);
	for (i=(nEvents-1); i>=0; i--)
	{
		Ticks = MidiEventGet(Sequence, Track, i, EVENT_TICKS);
		if (Ticks>Selection.left) continue;
		Event = MidiEventGet(Sequence, Track, i, EVENT_EVENT) & 0xF0;
		if (Event==0xC0)
		{
			if (Ticks==Selection.left)
			{
				MidiRemoveTrackEvents(Sequence, Track, i, i, MIDI_INDEX);
				Saved = FALSE;
			}
			else
				break;
		}
		if (Event==0xB0)
		{
			lpData = (LPBYTE) (DWORD) MidiEventGet(Sequence, Track, i, EVENT_DATA);
			if ((lpData[0]==0x00) || (lpData[0]==0x20))
			{
				MidiRemoveTrackEvents(Sequence, Track, i, i, MIDI_INDEX);
				Saved = FALSE;
			}
			MidiFreeBuffer(lpData);
		}

	}
	AdjustTrackChannels(Track);
}

LRESULT CALLBACK ProgramProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int Channel, Bank, BankHigh, BankLow, Program;
	BRELS_MIDI_EVENT Event[3];

	switch (uMsg)
	{
	case WM_CLOSE:
		EnableWindow(window, TRUE);
		ShowWindow(hwnd, SW_HIDE);
		return 0;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			Channel = SendMessage(GetDlgItem(hwnd, 0xFFFC), CB_GETCURSEL, 0, 0);
			Bank    = SendMessage(GetDlgItem(hwnd, 0xFFFD), CB_GETCURSEL, 0, 0);
			Program = SendMessage(GetDlgItem(hwnd, 0xFFFE), CB_GETCURSEL, 0, 0);
			if ((Channel==-1) || (Program==-1) || (Bank==-1)) break;
			MidiSuspend(Sequence, TRUE);
			MidiSilence(Sequence);
			RemoveProgram();
			ZeroMemory(&Event, sizeof(Event));
			BankHigh = 0x79;
			BankLow = Bank;
			if (Bank==10) { BankHigh = 0x78; BankLow = 0; }
			Event[0].dwTicks = Selection.right;
			Event[0].Event = 0xB0 | Channel;
			Event[0].DataSize = 2;
			Event[0].Data.a[0] = 0x00;
			Event[0].Data.a[1] = BankHigh;
			Event[1].dwTicks = Selection.right;
			Event[1].Event = 0xB0 | Channel;
			Event[1].DataSize = 2;
			Event[1].Data.a[0] = 0x20;
			Event[1].Data.a[1] = BankLow;
			Event[2].dwTicks = Selection.right;
			Event[2].Event = 0xC0 | Channel;
			Event[2].DataSize = 1;
			Event[2].Data.a[0] = Program;
			MidiInsertTrackEvents(Sequence, Track, 3, (LPBRELS_MIDI_EVENT) &Event);
			AdjustTrackChannels(Track);
			Saved = FALSE;
			MidiResume(Sequence, TRUE);
			AdjustWindow();
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		case IDCANCEL:
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		case 0xFFFC: if (HIWORD(wParam)==CBN_SELCHANGE)	ChannelList(hwnd, SendMessage((HWND) lParam, CB_GETCURSEL, 0, 0)); break;
		case 0xFFFD: if (HIWORD(wParam)==CBN_SELCHANGE)	ProgramList(hwnd, SendMessage((HWND) lParam, CB_GETCURSEL, 0, 0)); break;
		case 0xFFFF:
			MidiSuspend(Sequence, TRUE);
			MidiSilence(Sequence);
			RemoveProgram();
			MidiResume(Sequence, TRUE);
			AdjustWindow();
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		default:
			break;
		}
		return 0;
	default:
		break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void ChannelList(HWND hwnd, int Channel)
{
	int i, Bank, Program, BankHigh, BankLow;
	int Channels[16], Programs[16], BankHighs[16], BankLows[16];

	SendMessage(GetDlgItem(hwnd, 0xFFFC), CB_RESETCONTENT, 0, 0);

	for (i=0; i<16; i++)
	{
		Channels[i] = FALSE;
		BankHighs[i] = -1;
		BankLows[i] = -1;
		Programs[i] = -1;
		Channels[i] = GetChannelProgramInformation(i, Selection.right, &BankHigh, &BankLow, &Program);
		if (!Channels[i]) Channels[i] = GetChannelProgramInformation(i, -1, &BankHigh, &BankLow, &Program);
		if (Channels[i])
		{
			BankHighs[i] = BankHigh;
			BankLows[i] = BankLow;
			Programs[i] = Program;
		}

		strcpy(lpstr1, Strings[9]);
		if (i<10) strcat(lpstr1, "0");
		strcat(lpstr1, itoa(i, lpstr2, 10));
		if (i==9) strcpy(lpstr1, Strings[10]);
		if (Channels[i])
		{
			GetInstrumentName(BankHighs[i], BankLows[i], Programs[i], lpstr2);
			if (!strlen(lpstr2)) strcpy(lpstr2, Strings[11]);
			strcat(lpstr1, " (");
			strcat(lpstr1, lpstr2);
			strcat(lpstr1, ")");
		}
		SendMessage(GetDlgItem(hwnd, 0xFFFC), CB_ADDSTRING, 0, (LPARAM) lpstr1);
	}

	SendMessage(GetDlgItem(hwnd, 0xFFFC), CB_SETCURSEL, Channel, 0);
	SendMessage(GetDlgItem(hwnd, 0xFFFD), CB_SETCURSEL, -1, 0);
	SendMessage(GetDlgItem(hwnd, 0xFFFE), CB_SETCURSEL, -1, 0);
	if (Channel!=-1)
	{
		switch (BankHighs[Channel])
		{
		case -1: Bank = -1; break;
		case 0x78: Bank = 10; break;
		case 0x79: Bank = BankLows[Channel]; break;
		default: Bank = BankHighs[Channel]; break;
		}
		SendMessage(GetDlgItem(hwnd, 0xFFFD), CB_SETCURSEL, Bank, 0);
		ProgramList(hwnd, Bank);
		SendMessage(GetDlgItem(hwnd, 0xFFFE), CB_SETCURSEL, Programs[Channel], 0);
	}
}

void ProgramList(HWND hwnd, int Bank)
{
	int i, j;

	j = SendMessage(GetDlgItem(hwnd, 0xFFFE), CB_GETCURSEL, 0, 0);
	SendMessage(GetDlgItem(hwnd, 0xFFFE), CB_RESETCONTENT, 0, 0);
	if (Bank==-1) Bank=0;
	for (i=0; i<128; i++)
		SendMessage(GetDlgItem(hwnd, 0xFFFE), CB_ADDSTRING, 0, (LPARAM) &Instruments[Bank][i]);
	SendMessage(GetDlgItem(hwnd, 0xFFFE), CB_SETCURSEL, j, 0);
}

void PlaceProgram(void)
{
	BOOL Show;
	int i, Channel, Bank, BankHigh, BankLow, Program;
	HWND ProgramWindow;
	BRELS_MIDI_EVENT Events[3];

	memset(&Events, 0, sizeof(Events));
	Events[0].Event = 0xB0;
	Events[0].DataSize = 2;
	Events[0].dwTicks = Selection.right;
	Events[1].Event = 0xB0;
	Events[1].dwTicks = Selection.right;
	Events[1].DataSize = 2;
	Events[2].Event = 0xC0;
	Events[2].dwTicks = Selection.right;
	Events[2].DataSize = 1;

	GetProgramInformation(Track, Selection.right, &Channel, &BankHigh, &BankLow, &Program);
	if ((BankHigh!=0x79) && (BankHigh!=0x78) && (BankHigh!=-1))
	{
		BankLow = BankHigh;
		BankHigh = 0x79;
	}

	Show = /*((Program==-1) || (*/Selection.right==Selection.left/*))*/;
	if (!Show) /* Drag'n'drop a program icon */
	{
		GetProgramInformation(Track, Selection.left, &Channel, &BankHigh, &BankLow, &Program);
		RemoveProgram();
		if (BankLow==-1) BankLow = 0;
		if (BankHigh==-1) BankHigh = 0x79;
		Events[0].Event|=Channel; Events[0].Data.a[0]=0x00; Events[0].Data.a[1]=BankHigh;
		Events[1].Event|=Channel; Events[1].Data.a[0]=0x20; Events[1].Data.a[1]=BankLow;
		Events[2].Event|=Channel; Events[2].Data.a[0]=Program;
		MidiInsertTrackEvents(Sequence, Track, 3, (LPBRELS_MIDI_EVENT) &Events);
		AdjustTrackChannels(Track);
		Saved = FALSE;
		return;
	}

	ProgramWindow = CreateWindow("PROGRAMWINDOW", Strings[12], WS_POPUP | WS_CAPTION | WS_SYSMENU, GetSystemMetrics(SM_CXSCREEN) / 2 - 124, GetSystemMetrics(SM_CYSCREEN) / 2 - 58, 248+2*GetSystemMetrics(SM_CXFIXEDFRAME), 116+2*GetSystemMetrics(SM_CYFIXEDFRAME)+GetSystemMetrics(SM_CYCAPTION), window, NULL, wc.hInstance, NULL);
	CreateWindow("COMBOBOX", "", WS_TABSTOP | WS_VSCROLL | WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 4, 4, 240, 240, ProgramWindow, (HMENU) 0xFFFC, wc.hInstance, NULL);
	CreateWindow("COMBOBOX", "", WS_TABSTOP | WS_VSCROLL | WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 4, 32, 240, 240, ProgramWindow, (HMENU) 0xFFFD, wc.hInstance, NULL);
	CreateWindow("COMBOBOX", "", WS_TABSTOP | WS_VSCROLL | WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 4, 60, 240, 240, ProgramWindow, (HMENU) 0xFFFE, wc.hInstance, NULL);
	CreateWindow("BUTTON", Strings[13], WS_TABSTOP | WS_DISABLED | WS_CHILD | WS_VISIBLE, 4, 88, 64, 24, ProgramWindow, (HMENU) 0xFFFF, wc.hInstance, NULL);
	CreateWindow("BUTTON", Strings[14], WS_TABSTOP | WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 112, 88, 64, 24, ProgramWindow, (HMENU) IDOK, wc.hInstance, NULL);
	CreateWindow("BUTTON", Strings[15], WS_TABSTOP | WS_CHILD | WS_VISIBLE, 180, 88, 64, 24, ProgramWindow, (HMENU) IDCANCEL, wc.hInstance, NULL);
	for (i=0; i<10; i++)
	{
		strcpy(lpstr1, Strings[16]);
		strcat(lpstr1, itoa(i, lpstr2, 10));
		SendMessage(GetDlgItem(ProgramWindow, 0xFFFD), CB_ADDSTRING, 0, (LPARAM) lpstr1);
	}
	SendMessage(GetDlgItem(ProgramWindow, 0xFFFD), CB_ADDSTRING, 0, (LPARAM) Strings[10]);

	if (Program!=-1)
	{
		SetWindowText(ProgramWindow, Strings[17]);
		EnableWindow(GetDlgItem(ProgramWindow, 0xFFFF), TRUE);
	}
	switch (BankHigh)
	{
	case -1: Bank = -1; break;
	case 0x78: Bank = 10; break;
	case 0x79: Bank = BankLow; break;
	default: Bank = BankHigh; break;
	}
	ChannelList(ProgramWindow, Channel);
	ProgramList(ProgramWindow, Bank);
	SendMessage(GetDlgItem(ProgramWindow, 0xFFFC), CB_SETCURSEL, Channel, 0);
	SendMessage(GetDlgItem(ProgramWindow, 0xFFFD), CB_SETCURSEL, Bank, 0);
	SendMessage(GetDlgItem(ProgramWindow, 0xFFFE), CB_SETCURSEL, Program, 0);

	EnumChildWindows(ProgramWindow, EnumChildProc, 0);

	EnableWindow(window, FALSE);
	ShowWindow(ProgramWindow, SW_SHOW);
	SetFocus(GetDlgItem(ProgramWindow, 0xFFFC));

	while (IsWindowVisible(ProgramWindow))
		if (GetMessage(&msg, NULL, 0, 0)>0)
			if (!IsDialogMessage(GetActiveWindow(), &msg))
				DispatchMessage(&msg);
}

LRESULT CALLBACK TempoProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int i;
	switch (uMsg)
	{
	case WM_CLOSE:
		EnableWindow(window, TRUE);
		ShowWindow(hwnd, FALSE);
		return 0;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			MidiSuspend(Sequence, TRUE);
			MidiSilence(Sequence);
			MetaAction(META_DELETE, Track, Selection.left, FILTER_TEMPO, NULL, 0);
			i = SendMessage(GetDlgItem(hwnd, 0xFFFE), UDM_GETPOS32, 0, 0);
			lpstr1[0] = i >> 16;
			lpstr1[1] = (i >> 8) & 0xFF;
			lpstr1[2] = i & 0xFF;
			MetaAction(META_INSERT, Track, Selection.left, FILTER_TEMPO, lpstr1, 3);
			MidiResume(Sequence, TRUE);
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			AdjustWindow();
			break;
		case IDCANCEL:
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		case 0xFFFF:
			MidiSuspend(Sequence, TRUE);
			MidiSilence(Sequence);
			MetaAction(META_DELETE, Track, Selection.left, FILTER_TEMPO, NULL, 0);
			MidiResume(Sequence, TRUE);
			AdjustWindow();
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		default:
			break;
		}
		return 0;
	case WM_VSCROLL:
		if (lParam == (LPARAM) GetDlgItem(hwnd, 0xFFFB))
			SendMessage(GetDlgItem(hwnd, 0xFFFE), UDM_SETPOS32, 0, 60000000 / SendMessage((HWND) lParam, UDM_GETPOS32, 0, 0));
		if (lParam == (LPARAM) GetDlgItem(hwnd, 0xFFFE))
			SendMessage(GetDlgItem(hwnd, 0xFFFB), UDM_SETPOS32, 0, 60000000 / SendMessage((HWND) lParam, UDM_GETPOS32, 0, 0));
		return 0;
	default:
		break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void PlaceTempo(void)
{
	int i;
	HWND TempoWindow;
	BOOL Show, Preexistent;

	Preexistent = MetaAction(META_SELECT, Track, Selection.left, FILTER_TEMPO, lpstr1, 256);
	Show = !Preexistent || (Selection.right==Selection.left);
	if (!Show)
	{
		MetaAction(META_DELETE, Track, Selection.left, FILTER_TEMPO, NULL, 0);
		MetaAction(META_INSERT, Track, Selection.right, FILTER_TEMPO, lpstr1, 3);
		Saved = FALSE;
		return;
	}

	strcpy(lpstr3, Strings[18]);
	if (Preexistent)
	{
		i = (BYTE) lpstr1[0] * 65536 + (BYTE) lpstr1[1] * 256 + (BYTE) lpstr1[2];
		strcpy(lpstr3, Strings[19]);
	}
	else
		if (MetaAction(META_SELECT, Track, -1, FILTER_TEMPO, lpstr1, 256))
			i = (BYTE) lpstr1[0] * 65536 + (BYTE) lpstr1[1] * 256 + (BYTE) lpstr1[2];
		else
			i = 500000;
	itoa(60000000 / i, lpstr1, 10);
	itoa(i, lpstr2, 10);

	TempoWindow = CreateWindow("TEMPOWINDOW", lpstr3, WS_CAPTION | WS_SYSMENU | WS_POPUP, GetSystemMetrics(SM_CXSCREEN) / 2 - 118, GetSystemMetrics(SM_CYSCREEN) / 2 - 40, 236+2*GetSystemMetrics(SM_CXFIXEDFRAME), 80+GetSystemMetrics(SM_CYCAPTION)+2*GetSystemMetrics(SM_CYFIXEDFRAME), window, NULL, wc.hInstance, NULL);
	CreateWindow("STATIC", Strings[20], WS_VISIBLE | WS_CHILD, 4, 4, 144, 20, TempoWindow, (HMENU) 0xFFF9, wc.hInstance, NULL);
	CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", lpstr1, WS_TABSTOP | ES_READONLY | WS_VISIBLE | WS_CHILD, 168-GetSystemMetrics(SM_CXVSCROLL), 4, 64, 20, TempoWindow, (HMENU) 0xFFFA, wc.hInstance, NULL);
	CreateWindow(UPDOWN_CLASS, "", WS_CHILD | WS_VISIBLE | UDS_ARROWKEYS | UDS_SETBUDDYINT | UDS_NOTHOUSANDS, 232-GetSystemMetrics(SM_CXVSCROLL), 4, GetSystemMetrics(SM_CXVSCROLL), 20, TempoWindow, (HMENU) 0xFFFB, wc.hInstance, NULL);
	CreateWindow("STATIC", Strings[21], WS_VISIBLE | WS_CHILD, 4, 28, 144, 20, TempoWindow, (HMENU) 0xFFFC, wc.hInstance, NULL);
	CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", lpstr2, WS_TABSTOP | ES_READONLY | WS_VISIBLE | WS_CHILD, 168-GetSystemMetrics(SM_CXVSCROLL), 28, 64, 20, TempoWindow, (HMENU) 0xFFFD, wc.hInstance, NULL);
	CreateWindow(UPDOWN_CLASS, "", WS_CHILD | WS_VISIBLE | UDS_ARROWKEYS | UDS_SETBUDDYINT | UDS_NOTHOUSANDS, 232-GetSystemMetrics(SM_CXVSCROLL), 28, GetSystemMetrics(SM_CXVSCROLL), 20, TempoWindow, (HMENU) 0xFFFE, wc.hInstance, NULL);
	CreateWindow("BUTTON", Strings[13], WS_TABSTOP | WS_CHILD | WS_VISIBLE, 4, 52, 64, 24, TempoWindow, (HMENU) 0xFFFF, wc.hInstance, NULL);
	CreateWindow("BUTTON", Strings[14], BS_DEFPUSHBUTTON | WS_TABSTOP | WS_CHILD | WS_VISIBLE, 100, 52, 64, 24, TempoWindow, (HMENU) IDOK, wc.hInstance, NULL);
	CreateWindow("BUTTON", Strings[15], WS_TABSTOP | WS_CHILD | WS_VISIBLE, 168, 52, 64, 24, TempoWindow, (HMENU) IDCANCEL, wc.hInstance, NULL);
	SendMessage(GetDlgItem(TempoWindow, 0xFFFB), UDM_SETBUDDY, (WPARAM) GetDlgItem(TempoWindow, 0xFFFA), 0);
	SendMessage(GetDlgItem(TempoWindow, 0xFFFB), UDM_SETRANGE32, 4, 60000000);
	SendMessage(GetDlgItem(TempoWindow, 0xFFFE), UDM_SETBUDDY, (WPARAM) GetDlgItem(TempoWindow, 0xFFFD), 0);
	SendMessage(GetDlgItem(TempoWindow, 0xFFFE), UDM_SETRANGE32, 1, 16777215);
	EnableWindow(GetDlgItem(TempoWindow, 0xFFFF), Preexistent);

	EnumChildWindows(TempoWindow, EnumChildProc, 0);

	EnableWindow(window, FALSE);
	ShowWindow(TempoWindow, SW_SHOW);
	SetFocus(GetDlgItem(TempoWindow, 0xFFFA));
	while (IsWindowVisible(TempoWindow))
		if (GetMessage(&msg, NULL, 0, 0)>0)
			if (!IsDialogMessage(GetActiveWindow(), &msg))
				DispatchMessage(&msg);
}

void CopyEvents(void)
{
	int i, nEvents, Count;
	LPBRELS_MIDI_EVENT lpEvents;

	nEvents = MidiTrackGet(Sequence, Track, EVENT_COUNT);
	MidiGetTrackEvents(Sequence, Track, 0, nEvents-1, MIDI_INDEX, &lpEvents);
	Count = 0;
	for (i=0; i<nEvents; i++) if (lpEvents[i].wTag) Count++;

	if (Count)
	{
		ClipboardBeat = Beat;
		nClipboard = 0;
		MidiFreeBuffer(Clipboard);
		Clipboard = (LPBRELS_MIDI_EVENT) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Count*sizeof(BRELS_MIDI_EVENT));
		for (i=0; i<nEvents; i++)
			if (lpEvents[i].wTag)
				Clipboard[nClipboard++] = lpEvents[i];
	}

	MidiCleanEvents(nEvents, lpEvents);
}

void DeleteEvents(void)
{
	int i, j, nEvents;
	LPBRELS_MIDI_EVENT lpEvents, lpKept;

	MidiSuspend(Sequence, TRUE);
	MidiSilence(Sequence);
	nEvents = MidiTrackGet(Sequence, Track, EVENT_COUNT);
	MidiGetTrackEvents(Sequence, Track, 0, nEvents-1, MIDI_INDEX, &lpEvents);
	lpKept = (LPBRELS_MIDI_EVENT) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nEvents * sizeof(BRELS_MIDI_EVENT));
	j = 0;
	for (i=0; i<nEvents; i++)
		if (!lpEvents[i].wTag) lpKept[j++]=lpEvents[i];
	MidiRemoveTrack(Sequence, Track);
	MidiInsertTrack(Sequence, Track);
	MidiInsertTrackEvents(Sequence, Track, j, lpKept);
	MidiCleanEvents(nEvents, lpEvents);
	VirtualFree(lpKept, 0, MEM_RELEASE);
	MidiResume(Sequence, TRUE);
	AdjustScroll();
	Saved = FALSE;
}

void PasteEvents(void)
{
	int i, Base, ChannelNum;
	LPBRELS_MIDI_EVENT lpEvents;

	if (nClipboard)
	{
		MidiSuspend(Sequence, TRUE);
		MidiSilence(Sequence);
		/* Deselects events */
		for (i=0; i<(int) MidiTrackGet(Sequence, Track, EVENT_COUNT); i++)
			MidiEventSet(Sequence, Track, i, EVENT_TAG, FALSE);
		Base = MidiGet(Sequence, CURRENT_TICKS);
		GetProgramInformation(Track, Base, &ChannelNum, NULL, NULL, NULL);
		lpEvents = (LPBRELS_MIDI_EVENT) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BRELS_MIDI_EVENT)*nClipboard);
		CopyMemory(lpEvents, Clipboard, nClipboard*sizeof(BRELS_MIDI_EVENT));
		for (i=0; i<nClipboard; i++)
		{
			int type = lpEvents[i].Event & 0xF0;
			if ((type)==0x80)
				lpEvents[i].dwTicks = Base + ceil(((Clipboard[i].dwTicks-Clipboard[0].dwTicks) * (double) Beat) / ClipboardBeat);
			else
				lpEvents[i].dwTicks = Base + floor(((Clipboard[i].dwTicks-Clipboard[0].dwTicks) * (double) Beat) / ClipboardBeat);
		}
		MidiInsertTrackEvents(Sequence, Track, nClipboard, lpEvents);
		MidiCleanEvents(nClipboard, lpEvents);
		MidiResume(Sequence, TRUE);
		AdjustTrackChannels(Track);
		AdjustScroll();
		Saved = FALSE;
	}
}

void SelectAllEvents(void)
{
	int i, nEvents;
	LPBRELS_MIDI_EVENT lpEvents;

	nEvents = MidiTrackGet(Sequence, Track, EVENT_COUNT);
	MidiGetTrackEvents(Sequence, Track, 0, nEvents-1, MIDI_INDEX, &lpEvents);
	for (i=0; i<nEvents; i++)
		MidiEventSet(Sequence, Track, i, EVENT_TAG, ((lpEvents[i].Event>=0x80) && (lpEvents[i].Event<=0x9F)));
	MidiCleanEvents(nEvents, lpEvents);
}

int SelectEvents(void)
{
	BOOL Deselect, MultiSelect, Moving;
	int i, nEvents, nFilters, Selected, left, right, top, bottom;
	int Byte1, Ticks1, Ticks2, NewMovement;
	LPBRELS_MIDI_EVENT lpEvents;
	LPBRELS_FILTERED_EVENT lpFilters;

	if (Selection.top < Selection.bottom)
		{ top = Selection.bottom; bottom = Selection.top; }
	else
		{ bottom = Selection.bottom; top = Selection.top; }
	if (Selection.left > Selection.right)
		{ right = Selection.left; left = Selection.right; }
	else
		{ left = Selection.left; right = Selection.right; }

	nEvents = MidiTrackGet(Sequence, Track, EVENT_COUNT);
	MidiGetTrackEvents(Sequence, Track, 0, nEvents-1, MIDI_INDEX, &lpEvents);
	nFilters = MidiFilterTrackEvents(Sequence, Track, 0, nEvents-1, MIDI_INDEX, FILTER_NOTES, &lpFilters);

	Deselect = (GetAsyncKeyState(VK_CONTROL)>>15) != 0;
	MultiSelect = (GetAsyncKeyState(VK_SHIFT)>>15) != 0;
	NewMovement = SELECTING;

	/* Clicking note */
	Selected = -1;
	Moving = FALSE;
	if (Movement==0)
	for (i=0; i<nFilters; i++)
	{
		Ticks1 = lpEvents[lpFilters[i].qwStart].dwTicks;
		Ticks2 = lpEvents[lpFilters[i].qwEnd  ].dwTicks;
		Byte1  = lpEvents[lpFilters[i].qwStart].Data.a[0];
		if ((left>=Ticks1) && (left<=Ticks2) && (Selection.top==Byte1))
		{
			Selected = i;
			if ((left==Ticks1) || (left==Ticks2))
				NewMovement = RESIZING;
			else
				NewMovement = TRANSPOSING;
			MidiEventSet(Sequence, Track, lpFilters[i].qwStart, EVENT_TAG, !Deselect);
			MidiEventSet(Sequence, Track, lpFilters[i].qwEnd  , EVENT_TAG, !Deselect);
			if (lpEvents[lpFilters[i].qwStart].wTag)
				Moving = TRUE;
		}
	}

	/* Other notes */
	if ((!Moving) && ((Movement==0) || (Movement==SELECTING)))
	for (i=0; i<nFilters; i++)
	{
		Ticks1 = lpEvents[lpFilters[i].qwStart].dwTicks;
		Ticks2 = lpEvents[lpFilters[i].qwEnd  ].dwTicks;
		Byte1  = lpEvents[lpFilters[i].qwStart].Data.a[0];
		if ((Ticks1>=left) && (Ticks2<=right) && (Byte1<=top) && (Byte1>=bottom))
		{
			MidiEventSet(Sequence, Track, lpFilters[i].qwStart, EVENT_TAG, !Deselect);
			MidiEventSet(Sequence, Track, lpFilters[i].qwEnd  , EVENT_TAG, !Deselect);
		}
		else
		if (!MultiSelect && !Deselect && (i!=Selected))
		{
			MidiEventSet(Sequence, Track, lpFilters[i].qwStart, EVENT_TAG, FALSE);
			MidiEventSet(Sequence, Track, lpFilters[i].qwEnd  , EVENT_TAG, FALSE);
		}
	}

	MidiCleanEvents(nEvents, lpEvents);
	MidiFreeBuffer(lpFilters);

	return NewMovement;
}

void ChangeDevice(void)
{
	HMIDIOUT hmo;

	Device = SendMessage(GetDlgItem(window, ID_DEVICE), CB_GETCURSEL, 0, 0) - 1;
	MidiSet(Sequence, MIDI_HANDLE, (QWORD) (DWORD) NULL);
	midiOutOpen(&hmo, Device, (DWORD) NULL, (DWORD) NULL, (DWORD) CALLBACK_NULL);
	MidiSet(Sequence, MIDI_HANDLE, (QWORD) (DWORD) hmo);
	MidiSet(Sequence, CURRENT_TICKS, MidiGet(Sequence, CURRENT_TICKS));
}

void ChangeBeat(void)
{
	BOOL Preserve;
	DWORD dwTicks;
	int j, Tracks;
	WORD NewBeat, OldBeat;
	double Factor;

	MidiSuspend(Sequence, TRUE);
	MidiSilence(Sequence);
	Preserve = (GetAsyncKeyState(VK_SHIFT) >> 15)==0;
	OldBeat = Beat;
	NewBeat = (SendMessage(GetDlgItem(window, ID_BEAT), CB_GETCURSEL, 0, 0)+1) << 4;
	Beat = NewBeat;
	dwTicks = MidiGet(Sequence, CURRENT_TICKS);
	MidiSet(Sequence, BEAT_SIZE, Beat);
	Tracks = MidiGet(Sequence, TRACK_COUNT);
	Factor = (double) NewBeat / (double) OldBeat;

	if (Preserve)
	{
		CurrentPosition = CurrentPosition * Factor;
		dwTicks = dwTicks * Factor;
		for (j=0; j<Tracks; j++) ResizeTrack(j, NewBeat, OldBeat);
	}

	MidiSet(Sequence, CURRENT_TICKS, dwTicks);
	MidiResume(Sequence, TRUE);
	AdjustWindow();

	SetScrollPos(GetDlgItem(window, ID_POSITION), SB_CTL, CurrentPosition, TRUE);
	Saved = FALSE;
}

void InsertTrack(void)
{
	BRELS_MIDI_EVENT Events[2];

	MidiSuspend(Sequence, TRUE);
	MidiSilence(Sequence);
	Track = MidiGet(Sequence, TRACK_COUNT);
	MidiInsertTrack(Sequence, Track);
	ZeroMemory(&Events, sizeof(Events));
	strcpy(lpstr1, Strings[22]);
	strcat(lpstr1, itoa(Track+1, lpstr2, 10));
	Events[0].Event = 0x03;
	Events[0].DataSize = strlen(lpstr1);
	Events[0].Data.p = (LPBYTE) lpstr1;
	Events[1].Event = 0x00;
	Events[1].DataSize = 2;
	Events[1].Data.p = (LPBYTE) lpstr2;
	lpstr2[0] = Track >> 8;
	lpstr2[1] = Track & 0xFF;
	MidiInsertTrackEvents(Sequence, Track, 2, (LPBRELS_MIDI_EVENT) &Events);
	MidiResume(Sequence, TRUE);
	AdjustWindow();
	Saved = FALSE;
}

void RemoveTrack(void)
{
	WORD wTracks;

	if (MessageBox(window, Strings[23], Strings[24], MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2)==IDNO) return;

	if (MidiGet(Sequence, TRACK_COUNT)==1)
	{
		MessageBox(window, Strings[25], Strings[26], MB_ICONINFORMATION);
		return;
	}

	MidiSuspend(Sequence, TRUE);
	MidiSilence(Sequence);
	MidiRemoveTrack(Sequence, Track);
	wTracks = MidiGet(Sequence, TRACK_COUNT);
	if (Track>=wTracks) Track = wTracks -1;
	RenumberTracks();
	MidiResume(Sequence, TRUE);
	AdjustWindow();
	Saved = FALSE;
}

void Callback(void)
{
	DWORD dwTicks;

	if (MidiGet(Sequence, MIDI_STATUS)==MIDI_PLAY)
	{
		dwTicks = abs(CurrentPosition-MidiGet(Sequence, CURRENT_TICKS));

		if ((((int) dwTicks)>=PageSize) && (((int) dwTicks)<=(2*PageSize)) && (!Positioning))
		{
			CurrentPosition = MidiGet(Sequence, CURRENT_TICKS);
			SetScrollPos(GetDlgItem(window, ID_POSITION), SB_CTL, CurrentPosition, TRUE);
		}

		if (!PlayButtonsAdjusted)
		{
			PlayButtonsAdjusted = TRUE;
			AdjustButtons();
		}

		Redraw();
	}
	else
	{
		PlayButtonsAdjusted = FALSE;
		AdjustButtons();
		Redraw();
	}
}

BOOL FileNew(void)
{
	HANDLE NewSequence;
	HMIDIOUT hmo;
	BRELS_MIDI_EVENT Events[10];

	memset(&Events, 0, sizeof(Events));
	Events[0].Event = 0x00;	Events[0].DataSize = 2;
	Events[1].Event = 0x02;	Events[1].DataSize = 0;
	Events[2].Event = 0x03;	Events[2].DataSize = 0;
	Events[3].Event = 0x51;	Events[3].DataSize = 3;
	Events[4].Event = 0xB0;	Events[4].DataSize = 2;
	Events[5].Event = 0xB0;	Events[5].DataSize = 2;
	Events[6].Event = 0xB0;	Events[6].DataSize = 2;
	Events[7].Event = 0xB0;	Events[7].DataSize = 2;
	Events[8].Event = 0xB0;	Events[8].DataSize = 2;
	Events[9].Event = 0xC0;	Events[9].DataSize = 1;

	if (CheckSave(Strings[27]))
	{
		if (!MidiCreate(NO_DEVICE, 16, &NewSequence))
		{
			MessageBox(window, Strings[28], Strings[29], MB_ICONERROR);
			return FALSE;
		}

		MidiClose(Sequence);
		Sequence = NewSequence;
		midiOutOpen(&hmo, Device, (DWORD) NULL, (DWORD) NULL, (DWORD) CALLBACK_NULL);
		MidiSet(Sequence, MIDI_HANDLE, (QWORD) (DWORD) hmo);
		Beat = MidiGet(Sequence, BEAT_SIZE);
		Zoom = 100;
		Track = 0;
		Controller = -2;
		Saved = TRUE;
		CurrentPosition = 0;
		CurrentNote = 64;
		PlayButtonsAdjusted = FALSE;
		strcpy(lpfile, "");

		MidiInsertTrack(Sequence, 0);
		Events[0].Data.p = (LPBYTE) &trackSignature[0];
		Events[1].Data.p = (LPBYTE) &Strings[30];
		Events[1].DataSize = strlen((const char*) Events[1].Data.p);
		Events[2].Data.p = (LPBYTE) &Strings[31];
		Events[2].DataSize = strlen((const char*) Events[2].Data.p);
		Events[3].Data.p = (LPBYTE) &defaultTempo[0];
		Events[4].Data.a[0] = 0x00;
		Events[4].Data.a[1] = 0x79;
		Events[5].Data.a[0] = 0x20;
		Events[5].Data.a[1] = 0x00;
		Events[6].Data.a[0] = 0x07;
		Events[6].Data.a[1] = 0x7F;
		Events[7].Data.a[0] = 0x0A;
		Events[7].Data.a[1] = 0x40;
		Events[8].Data.a[0] = 0x0B;
		Events[8].Data.a[1] = 0x7F;
		Events[9].Data.a[1] = 0x00;
		MidiInsertTrackEvents(Sequence, 0, 10, (LPBRELS_MIDI_EVENT) &Events);

		/* You can use it if you wish to */
		/* MidiSet(Sequence, CALLBACK_HWND, (QWORD) window); */
		/* MidiSet(Sequence, CALLBACK_MESSAGE, (QWORD) WM_CALLBACK); */

		AdjustWindow();
		SetScrollPos(GetDlgItem(window, ID_NOTE), SB_CTL, 64, TRUE);
		SetScrollPos(GetDlgItem(window, ID_POSITION), SB_CTL, 0, TRUE);
	}
	else return FALSE;

	return TRUE;
}

BOOL CheckSave(LPSTR Title)
{
	if (!Saved)
		switch (MessageBox(window, Strings[32], Title, MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFBUTTON3))
		{
		case IDYES: FileSave(lpfile); return TRUE;
		case IDNO: return TRUE;
		default: return FALSE;
		}
	else return TRUE;
}

BOOL FileOpen(void)
{
	HMIDIOUT hmo;
	HANDLE NewSequence;
	OPENFILENAME ofn;
	LPBRELS_MIDI_HEADER lpbmh;

	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME); /*OPENFILENAME_SIZE_VERSION_400; */
	ofn.hwndOwner = window;
	ofn.lpstrFilter = Strings[33];
	ofn.lpstrFile = lpfile;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = Strings[34];

	if (CheckSave(Strings[35]))
	{
		if (GetOpenFileName(&ofn))
		{
			if (!MidiOpen(lpfile, NO_DEVICE, &NewSequence))
			{
				MessageBox(window, Strings[36], Strings[29], MB_ICONERROR);
				return FALSE;
			}
			MidiClose(Sequence);
			Sequence = NewSequence;
			midiOutOpen(&hmo, Device, (DWORD) NULL, (DWORD) NULL, (DWORD) CALLBACK_NULL);
			MidiSet(Sequence, MIDI_HANDLE, (QWORD) (DWORD) hmo);

			/* You can use it if you wish to */
			/* MidiSet(Sequence, CALLBACK_HWND, (QWORD) window); */
			/* MidiSet(Sequence, CALLBACK_MESSAGE, (QWORD) WM_CALLBACK); */
			Beat = MidiGet(Sequence, BEAT_SIZE);
			Saved = TRUE;
			PlayButtonsAdjusted = FALSE;
			Zoom = 100;
			Track = 0;
			Controller = -2;
			CurrentPosition = 0;
			CurrentNote = 64;
			if ((Beat>16) && (FirstFileOpen))
			{
				MessageBox(window, Strings[37], Strings[38], MB_ICONINFORMATION);
				FirstFileOpen = FALSE;
			}
			lpbmh = (LPBRELS_MIDI_HEADER) (DWORD) MidiGet(Sequence, MIDI_HEADER);
			if (btlw(lpbmh->wFormat)==0) SplitChannels();
			MidiFreeBuffer(lpbmh);

			AdjustWindow();
			SetScrollPos(GetDlgItem(window, ID_NOTE), SB_CTL, 64, TRUE);
			SetScrollPos(GetDlgItem(window, ID_POSITION), SB_CTL, 0, TRUE);
		}
		else return FALSE;
	}

	return TRUE;
}

BOOL FileSave(LPSTR lpstrFile)
{
	OPENFILENAME ofn;

	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof (OPENFILENAME); /*OPENFILENAME_SIZE_VERSION_400 ; */
	ofn.hwndOwner = window;
	ofn.lpstrFilter = Strings[33];
	ofn.lpstrFile = lpfile;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = Strings[34];

	if (!PathFileExists(lpstrFile))
		if (!GetSaveFileName(&ofn)) return FALSE;

	MidiEncode(Sequence, lpfile, TRUE);
	Saved = TRUE;

	AdjustWindow();

	return TRUE;
}

void WindowSize(WPARAM wParam, LPARAM lParam)
{
	int x, y;

	UNREFERENCED_PARAMETER(wParam);

	x = LOWORD(lParam);
	y = HIWORD(lParam);

	Ruler.top = 80;
	Ruler.bottom = 100;
	Ruler.left = 0;
	Ruler.right = x;
	Bar.top = Ruler.bottom;
	Bar.bottom = 120;
	Bar.left = 0;
	Bar.right = x;
	Piano.top = Bar.bottom;
	Piano.bottom = y;
	Piano.left = 0;
	Piano.right = 96;
	Controllers.top = y - 129;
	Controllers.bottom = y;
	Controllers.left = 96;
	Controllers.right = x;
	Notes.top = Piano.top;
	Notes.bottom = y - GetSystemMetrics(SM_CYHSCROLL) - 129;
	Notes.left = Piano.right;
	Notes.right = x - GetSystemMetrics(SM_CXVSCROLL);

	SetWindowPos(GetDlgItem(window, ID_POSITION), NULL, Notes.left, Notes.bottom, x - Notes.left, GetSystemMetrics(SM_CYHSCROLL), SWP_NOZORDER);
	SetWindowPos(GetDlgItem(window, ID_NOTE), NULL, Notes.right, Notes.top, GetSystemMetrics(SM_CXVSCROLL), Notes.bottom - Notes.top, SWP_NOZORDER);

	DeleteDC(buffer);
	DeleteObject(bitmap);
	buffer = CreateCompatibleDC(hdc);

	/* A compatible bitmap will cause distorted sound if graphic acceleration is turned on in XP/2000 */
	bitmap = CreateBitmap(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), GetDeviceCaps(hdc, PLANES), GetDeviceCaps(hdc, BITSPIXEL), NULL);

	SelectObject(buffer, bitmap);
	SelectObject(buffer, hfont);
	SetBkMode(buffer, TRANSPARENT);

	Redraw();
}

void WindowPaint(WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;

	UNREFERENCED_PARAMETER(lParam);
	UNREFERENCED_PARAMETER(wParam);

	BeginPaint(window, &ps);
	EndPaint(window, &ps);
	DoRedraw();
}

void WindowHScroll(WPARAM wParam, LPARAM lParam)
{
	SCROLLINFO si;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_ALL;
	GetScrollInfo((HWND) lParam, SB_CTL, &si);

	Positioning = TRUE;

	switch (LOWORD(wParam))
	{
	case SB_ENDSCROLL: Positioning = FALSE; break;
	case SB_LINELEFT: si.nPos-=Beat / 16; break;
	case SB_LINERIGHT: si.nPos+=Beat / 16; break;
	case SB_PAGELEFT: si.nPos-=PageSize; break;
	case SB_PAGERIGHT: si.nPos+=PageSize; break;
	case SB_THUMBTRACK: si.nPos=si.nTrackPos; break;
	case SB_THUMBPOSITION: si.nPos=si.nTrackPos; break;
	default: break;
	}

	if (si.nPos > si.nMax) si.nMax+=PageSize;
	if (si.nPos < 0) si.nPos = 0;

	CurrentPosition = si.nPos;
	SetScrollInfo((HWND) lParam, SB_CTL, &si, TRUE);
	Redraw();
}

void WindowVScroll(WPARAM wParam, LPARAM lParam)
{
	SCROLLINFO si;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_ALL;
	GetScrollInfo((HWND) lParam, SB_CTL, &si);

	switch (LOWORD(wParam))
	{
	case SB_LINELEFT: si.nPos-=1; break;
	case SB_LINERIGHT: si.nPos+=1; break;
	case SB_PAGELEFT: si.nPos-=12; break;
	case SB_PAGERIGHT: si.nPos+=12; break;
	case SB_THUMBTRACK: si.nPos=si.nTrackPos; break;
	case SB_THUMBPOSITION: si.nPos=si.nTrackPos; break;
	default: break;
	}

	if (si.nPos > si.nMax) si.nPos = si.nMax;
	if (si.nPos < 0) si.nPos = 0;

	CurrentNote = si.nPos;
	SetScrollInfo((HWND) lParam, SB_CTL, &si, TRUE);
	Redraw();
}

void WindowMouseMove(WPARAM wParam, LPARAM lParam)
{
	int Area, NewPosition, NewNote, Tick, tickCount;

	CalculateMeasures();
	Area = GetArea(wParam, lParam);
	RButton = (wParam & MK_RBUTTON) == MK_RBUTTON;
	LButton  = (wParam & MK_LBUTTON) == MK_LBUTTON;
	MouseX = LOWORD(lParam); MouseY = HIWORD(lParam);
	Selection.right  = CurrentPosition + (MouseX - Notes.left) / TickWidth;
	Selection.bottom = LastNote - floor((MouseY - Notes.top) / KeyHeight);

	if (!LButton) Movement = 0;

	if (Area==PIANO)
	{
		if (!LButton) SilentPiano();
		if ((LButton) && (PlayedNote!=Selection.bottom))
		{
			SilentPiano();
			PlayedNote = Selection.bottom;
			PlayPiano(PlayedNote);
		}
	}

	if (Movement)
	{
		if (Movement==SELECTING)
			SelectEvents();
		/*if (Movement==RESIZING) */
		/*	if (Selection.bottom!=Selection.top) Movement=TRANSPOSING; */
		Tick = BeatWidth / TickWidth / 16;
		NewPosition = CurrentPosition;
		tickCount = MidiGet(Sequence, TICK_COUNT);
		if (MouseX<Notes.left) NewPosition -= Tick;
		if (MouseX>(Notes.right-8)) NewPosition += Tick;
		if ((NewPosition!=CurrentPosition) && (NewPosition>=0))
		{
			Selection.right+=CurrentPosition - NewPosition;
			CurrentPosition = NewPosition;
			if (CurrentPosition>tickCount)
				SetScrollRange(GetDlgItem(window, ID_POSITION), SB_CTL, 0, CurrentPosition+PageSize, FALSE);
			SetScrollPos(GetDlgItem(window, ID_POSITION), SB_CTL, NewPosition, TRUE);
		}
		if (Movement!=DRAGGING)
		{
			NewNote = CurrentNote;
			if (MouseY<Notes.top) NewNote--;
			if (MouseY>Notes.bottom) NewNote++;
			if ((NewNote!=CurrentNote) && (NewNote>=0) && (NewNote<=127))
			{
				Selection.bottom+=CurrentNote - NewNote;
				CurrentNote = NewNote;
				SetScrollPos(GetDlgItem(window, ID_NOTE), SB_CTL, NewNote, TRUE);
			}
		}
	}
	else
		if (Area==CONTROLLERS)
		{
			if (LButton)
				UpdateController(MouseY);
			else
				if (RButton) DeleteController();
		}

	ChooseCursor(Area);

	if (MidiGet(Sequence, MIDI_STATUS)!=MIDI_PLAY)
		Redraw();
}

void WindowLButtonUp(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	UNREFERENCED_PARAMETER(wParam);

	SilentPiano();

	switch (Movement)
	{
	case DRAGGING:
		PlaceMeta();
		break;
	case TRANSPOSING:
		Transpose(Selection.right-Selection.left, Selection.bottom-Selection.top);
		break;
	case RESIZING:
		ResizeNote();
		break;
	case PLACING:
		PlaceNote((Beat << 3) >> (Mode-ID_0));
		break;
	default:
		break;
	}

	InController=FALSE;
	Movement = 0;
	Selection.left=-1;
	Selection.top=-1;
	Redraw();
}

void WindowLButtonDown(WPARAM wParam, LPARAM lParam)
{
	CalculateMeasures();
	Selection.left   = CurrentPosition + ((LOWORD(lParam)-Notes.left) / TickWidth);
	Selection.top    = LastNote - floor((HIWORD(lParam)-Piano.top) / KeyHeight);

	switch (GetArea(wParam, lParam))
	{
	case PIANO: PlayPiano(Selection.top); PlayedNote = Selection.top; break;
	case BAR:
		Movement = DRAGGING;
		break;
	case NOTES:
		if (Mode==ID_SELECT)
			Movement = SelectEvents();
		else
			Movement = PLACING;
		break;
	case RULER:
		if (Selection.right>=0)
		{
			MidiSuspend(Sequence, TRUE);
			MidiSilence(Sequence);
			MidiSet(Sequence, CURRENT_TICKS, Selection.right);
			MidiResume(Sequence, TRUE);
		}
		break;
	case CONTROLLERS:
		UpdateController(HIWORD(lParam));
		break;
	default:
		break;
	}

	SetFocus(window);
	Redraw();
}

void WindowRButtonUp(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	UNREFERENCED_PARAMETER(wParam);

	InController=FALSE;
	Movement = 0;
	Selection.left=-1;
	Selection.top=-1;
	Redraw();
}

void WindowRButtonDown(WPARAM wParam, LPARAM lParam)
{
	InController=FALSE;
	CalculateMeasures();
	Selection.left   = CurrentPosition + ((LOWORD(lParam)-Notes.left) / TickWidth);
	Selection.top    = LastNote - floor((HIWORD(lParam)-Piano.top) / KeyHeight);

	switch (GetArea(wParam, lParam))
	{
	case BAR: RemoveMeta(); break;
	case CONTROLLERS: DeleteController(); break;
	default: break;
	}

	SetFocus(window);
	Redraw();
}

void KeyUp(WPARAM wParam, LPARAM lParam)
{
	int i, Channel;
	HMIDIOUT hmo;

	UNREFERENCED_PARAMETER(lParam);

	if (Movement) return;
	if (MidiGet(Sequence, MIDI_STATUS)==MIDI_PLAY) return;

	i = NoteVK(wParam);
	if (i!=-1)
	{
		GetProgramInformation(Track, -1, &Channel, NULL, NULL, NULL);
		if (Channel!=-1)
		{
			hmo = (HMIDIOUT) (DWORD) MidiGet(Sequence, MIDI_HANDLE);
			midiOutShortMsg(hmo, (0x80 | Channel) | ((((127-CurrentNote)  / 12) * 12 + i)<<8));
		}
	}
}

void KeyDown(WPARAM wParam, LPARAM lParam)
{
	int i;

	switch (wParam)
	{
	case VK_ESCAPE: Movement = 0; Selection.left=-1; Selection.top=-1; break;
	case VK_LEFT  : Transpose(-Beat / 16, 0); break;
	case VK_RIGHT : Transpose(+Beat / 16, 0); break;
	case VK_UP    : Transpose(0, +1); break;
	case VK_DOWN  : Transpose(0, -1); break;
	case VK_PRIOR : Transpose(0, +12); break;
	case VK_NEXT  : Transpose(0, -12); break;
	case VK_HOME  : SendMessage(window, WM_COMMAND, ID_HOME, 0); break;
	case VK_END   : SendMessage(window, WM_COMMAND, ID_END , 0); break;
	case VK_DELETE: DeleteEvents(); break;
	case 'C'      : if (GetAsyncKeyState(VK_CONTROL)>>15) CopyEvents(); break;
	case 'V'      : if (GetAsyncKeyState(VK_CONTROL)>>15) PasteEvents(); break;
	case 'X'      : if (GetAsyncKeyState(VK_CONTROL)>>15) { CopyEvents(); DeleteEvents(); } break;
	case 'A'      : if (GetAsyncKeyState(VK_CONTROL)>>15) SelectAllEvents();
	default       :
		if ((wParam>='0') && (wParam<='9'))
		{
			CurrentNote = 127 - (wParam-'0') * 12;
			SetScrollPos(GetDlgItem(window, ID_NOTE), SB_CTL, CurrentNote, TRUE);
		}
		else
		{
			i = NoteVK(wParam);
			if ((i!=-1) && (((lParam >> 30) & 1)==0))
				PlayPiano(((127-CurrentNote)  / 12) * 12 + i);
			else
				return;
		}
		break;
	}

	Redraw();
}

void PlayPiano(int Note)
{
	int Program, Channel, BankHigh, BankLow;
	HMIDIOUT hmo;

	if (Movement) return;
	if (MidiGet(Sequence, MIDI_STATUS)==MIDI_PLAY) return;

	GetProgramInformation(Track, -1, &Channel, &BankHigh, &BankLow, &Program);
	if (Channel!=-1)
	{
		hmo = (HMIDIOUT) (DWORD) MidiGet(Sequence, MIDI_HANDLE);
		if (BankLow==-1) BankLow = 0;
		if (BankHigh==-1) BankHigh=0;
		if (Program==-1) Program = 0;
		midiOutShortMsg(hmo, (0xB0 | Channel) | (0x00 << 8) | (BankHigh << 16));
		midiOutShortMsg(hmo, (0xB0 | Channel) | (0x20 << 8) | (BankLow  << 16));
		midiOutShortMsg(hmo, (0xB0 | Channel) | (0x07 << 8) | (0x7F << 16));
		midiOutShortMsg(hmo, (0xB0 | Channel) | (0x0A << 8) | (0x40  << 16));
		midiOutShortMsg(hmo, (0xB0 | Channel) | (0x0B << 8) | (0x7F  << 16));
		midiOutShortMsg(hmo, (0xC0 | Channel) | (Program << 8));
		midiOutShortMsg(hmo, (0x90 | Channel) | (Note << 8) | (64 << 16));
	}
}

void SilentPiano(void)
{
	int i;
	HMIDIOUT hmo;

	if (Movement) return;
	if (MidiGet(Sequence, MIDI_STATUS)==MIDI_PLAY) return;

	hmo = (HMIDIOUT) (DWORD) MidiGet(Sequence, MIDI_HANDLE);
	for (i=0; i<16; i++) midiOutShortMsg(hmo, (0xB0 | i) | (120 << 8));
}

void CalculateMeasures(void)
{
	TickWidth = BeatWidth * Zoom / (100 * Beat);
	PageSize = (Notes.right - Notes.left) / TickWidth;
	KeyHeight = KeyBaseHeight * Zoom / 100;
	FirstNote = (127-CurrentNote) - (Controllers.bottom-Controllers.top) / KeyHeight - (Notes.bottom-Notes.top) / (2*KeyHeight);
	LastNote = (127-CurrentNote) + (Piano.bottom-Piano.top) / KeyHeight - (Controllers.bottom-Controllers.top) / KeyHeight - (Notes.bottom-Notes.top) / (2*KeyHeight);
	OffsetX = Selection.right - Selection.left;
	OffsetY = Selection.bottom - Selection.top;
}

int GetArea(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);

	if ((LOWORD(lParam)>=Ruler.left) && (LOWORD(lParam)<=Ruler.right) && (HIWORD(lParam)>=Ruler.top) && (HIWORD(lParam)<=Ruler.bottom)) return RULER;
	if ((LOWORD(lParam)>=Bar.left  ) && (LOWORD(lParam)<=Bar.right)   && (HIWORD(lParam)>=Bar.top  ) && (HIWORD(lParam)<=Bar.bottom)) return BAR;
	if ((LOWORD(lParam)>=Piano.left) && (LOWORD(lParam)<=Piano.right) && (HIWORD(lParam)>=Piano.top) && (HIWORD(lParam)<=Piano.bottom)) return PIANO;
	if ((LOWORD(lParam)>=Notes.left) && (LOWORD(lParam)<=Notes.right) && (HIWORD(lParam)>=Notes.top) && (HIWORD(lParam)<=Notes.bottom)) return NOTES;
	if ((LOWORD(lParam)>=Controllers.left) && (LOWORD(lParam)<=Controllers.right) && (HIWORD(lParam)>=Controllers.top) && (HIWORD(lParam)<=Controllers.bottom)) return CONTROLLERS;
	return 0;
}

void ChooseCursor(int Area)
{
	int OldCursor;

	OldCursor = CursorSet;
	switch (Area)
	{
	case RULER: CursorSet = UPARROW; break;
	case BAR  : CursorSet = HAND; break;
	case PIANO: CursorSet = HAND; break;
	case NOTES: if (Mode==ID_SELECT) CheckCursor(); else CursorSet = HAND; break;
	default   : CursorSet = ARROW; break;
	}

	if (CursorSet!=OldCursor)
	{
		SetClassLongPtr(window, GCLP_HCURSOR, (LONG_PTR) Cursors[CursorSet]);
		SetWindowPos(GetDlgItem(window, ID_FILENEW), NULL, 0, 0, 25, 24, SWP_NOMOVE | SWP_NOZORDER);
		SetWindowPos(GetDlgItem(window, ID_FILENEW), NULL, 0, 0, 24, 24, SWP_NOMOVE | SWP_NOZORDER);
	}
}

void CheckCursor(void)
{
	int i, nEvents, nFilters;
	LPBRELS_MIDI_EVENT lpEvents;
	LPBRELS_FILTERED_EVENT lpFilters;

	if (Movement == TRANSPOSING) { CursorSet = SIZEALL; return; }
	if (Movement == RESIZING   ) { CursorSet = SIZEWE ; return; }

	nEvents = MidiTrackGet(Sequence, Track, EVENT_COUNT);
	MidiGetTrackEvents(Sequence, Track, 0, nEvents-1, MIDI_INDEX, &lpEvents);
	nFilters = MidiFilterTrackEvents(Sequence, Track, 0, nEvents-1, MIDI_INDEX, FILTER_NOTES, &lpFilters);

	if (CursorSet!=ARROW) CursorSet = ARROW;

	for (i=0; i<nFilters; i++)
	{
		if ((lpEvents[lpFilters[i].qwStart].dwTicks==(DWORD) Selection.right) && (lpEvents[lpFilters[i].qwStart].Data.a[0]==(DWORD) Selection.bottom))
		{
			CursorSet = SIZEWE;
			break;
		}
		if ((lpEvents[lpFilters[i].qwEnd].dwTicks==(DWORD) Selection.right) && (lpEvents[lpFilters[i].qwEnd].Data.a[0]==(DWORD) Selection.bottom))
		{
			CursorSet = SIZEWE;
			break;
		}
		if ((Selection.right>=(int) lpEvents[lpFilters[i].qwStart].dwTicks) && (Selection.right<=(int) lpEvents[lpFilters[i].qwEnd].dwTicks) && (Selection.bottom==(int) lpEvents[lpFilters[i].qwStart].Data.a[0]))
		{
			CursorSet = SIZEALL;
			break;
		}
	}

	MidiFreeBuffer(lpFilters);
	MidiCleanEvents(nEvents, lpEvents);
}

void Redraw(void)
{
	MSG __msg;

	if (PeekMessage(&__msg, NULL, WM_PAINT, WM_PAINT, PM_NOREMOVE)==FALSE)
		if (PeekMessage(&__msg, NULL, WM_REDRAW, WM_REDRAW, PM_NOREMOVE)==FALSE)
				PostMessage(window, WM_REDRAW, 0, 0);
}

void DoRedraw(void)
{
	HPEN Pen;
	int i, j, Min, Sec, Ms, Key, Interval, Limit, Ticks, EndOfTrack;
	int nEvents, nFilters, Length, Last, Rows, Event, Byte1, Byte2, DataSize;
	int Start, End, Icon, Mask, BankHigh, BankLow, CurrentController;
	int left, top, bottom, right;
	BOOL Long, DrawBlock1, DrawBlock2, Selected;
	RECT Block1 = { 0, 0, 0, 0}, Block2 = { 0, 0, 0, 0 };
	LPBRELS_MIDI_EVENT lpEvents;
	LPBRELS_FILTERED_EVENT lpFilters;
	LPBYTE lpData;
	QWORD qwTime, qwFilter;
	SIZE size;

	CalculateMeasures();

	Ticks = MidiGet(Sequence, CURRENT_TICKS);
	EndOfTrack = MidiGet(Sequence, TICK_COUNT);

	/* Backgrounds */
	SelectObject(buffer, blackp);
	SelectObject(buffer, white );
	Rectangle(buffer, Ruler.left-1, Ruler.top, Ruler.right+1, Ruler.bottom);
	Rectangle(buffer, Bar.left-1, Bar.top, Bar.right+1, Bar.bottom);
	Rectangle(buffer, Notes.left, Notes.top, Notes.right, Notes.bottom);
	Rectangle(buffer, Controllers.left, Controllers.top, Controllers.right, Controllers.bottom);

	/* Time ruler and division */
	SetTextAlign(buffer, TA_CENTER);
	SetBkMode(buffer, OPAQUE);
	SetTextColor(buffer, 0x000000);
	Interval = BeatWidth / TickWidth;
	Last = -2*Interval;

	for (i=CurrentPosition-Interval; i<=(CurrentPosition+PageSize+Interval); i++)
	if (i>=0)
	{
		Start = Notes.left + (i-CurrentPosition)*TickWidth;
		if (((i % Beat) == 0) && (abs((i-Last)*TickWidth) >= BeatWidth))
		{
			Last = i;

			SelectObject(buffer, blackp);
			MoveToEx(buffer, Start, Ruler.top, NULL);
			LineTo(buffer, Start, Ruler.bottom);

			qwTime = MidiQuery(Sequence, 0, MIDI_TIME, MIDI_TICKS, i);
			Min = qwTime / 60000000;
			Sec = (qwTime / 1000000) % 60;
			Ms = (qwTime % 1000000) / 1000;
			strcpy(lpstr1, "");
			if (Min<10) strcat(lpstr1, "0");
			strcat(lpstr1, itoa(Min, lpstr2, 10));
			strcat(lpstr1, ":");
			if (Sec<10) strcat(lpstr1, "0");
			strcat(lpstr1, itoa(Sec, lpstr2, 10));
			strcat(lpstr1, ":");
			if (Ms<100) strcat(lpstr1, "0");
			if (Ms<10) strcat(lpstr1, "0");
			strcat(lpstr1, itoa(Ms, lpstr2, 10));
			TextOut(buffer, Start, Ruler.top + 4, lpstr1, strlen(lpstr1));

		}

		if ((i>=CurrentPosition) && (i<=CurrentPosition+PageSize))
		{
			if ((0==(int) fmod(i, (Beat        ))) && ((Beat * TickWidth     )>=4)) Pen = gray1; else
			if ((0==(int) fmod(i, (Beat /  2.0l))) && ((Beat * TickWidth /  2)>=4)) Pen = gray2; else
			if ((0==(int) fmod(i, (Beat /  4.0l))) && ((Beat * TickWidth /  4)>=4)) Pen = gray3; else
			if ((0==(int) fmod(i, (Beat /  8.0l))) && ((Beat * TickWidth /  8)>=4)) Pen = gray4; else
			if ((0==(int) fmod(i, (Beat / 16.0l))) && ((Beat * TickWidth / 16)>=4)) Pen = gray5; else
			continue;

			SelectObject(buffer, Pen);
			MoveToEx(buffer, Start, Notes.top+1, NULL);
			LineTo(buffer, Start, Notes.bottom-1);
		}

	}

	SetTextAlign(buffer, TA_LEFT);
	SetBkMode(buffer, TRANSPARENT);
	SelectObject(buffer, blackp);

	/* End of track line */
	if ((EndOfTrack>=CurrentPosition) && (EndOfTrack<=(CurrentPosition+PageSize)))
		Rectangle(buffer, Notes.left + (EndOfTrack-CurrentPosition)*TickWidth-1, Notes.top, Notes.left + (EndOfTrack-CurrentPosition)*TickWidth+1, Notes.bottom);

	/* Bar event */
	if (Movement==DRAGGING) Mask = ILD_SELECTED; else Mask = ILD_NORMAL;
	ImageList_Draw(imagelist, Meta, buffer, Notes.left + (Selection.right-CurrentPosition)*TickWidth - 4, Bar.top + 2, Mask);

	/* Notes, controllers and meta-events */
	Last = 0;
	Rows = 0;
	BankHigh = -1;
	BankLow = -1;
	if (InController)
		CurrentController = ControllerValue;
	else
		CurrentController = -1;
	Limit = CurrentPosition + PageSize + GetSystemMetrics(SM_CXVSCROLL) / TickWidth;
	nEvents = MidiTrackGet(Sequence, Track, EVENT_COUNT);
	MidiGetTrackEvents(Sequence, Track, 0, nEvents-1, MIDI_INDEX, &lpEvents);
	nFilters = MidiFilterTrackEvents(Sequence, Track, 0, nEvents-1, MIDI_INDEX, FILTER_ALL | FILTER_NOTES | FILTER_TEMPO, &lpFilters);

	for (i=0; i<nFilters; i++)
	{

		Start = lpEvents[lpFilters[i].qwStart].dwTicks;

		if (Start>Limit) break;

		Icon = -1;
		strcpy(lpstr1, "");
		DrawBlock1 = FALSE;
		DrawBlock2 = FALSE;
		qwFilter = lpFilters[i].qwFilter;

		Selected = lpEvents[lpFilters[i].qwStart].wTag;
		Event = lpEvents[lpFilters[i].qwStart].Event;
		Byte1 = lpEvents[lpFilters[i].qwStart].Data.a[0];
		Byte2 = lpEvents[lpFilters[i].qwStart].Data.a[1];
		End   = lpEvents[lpFilters[i].qwEnd  ].dwTicks;
		DataSize = lpEvents[lpFilters[i].qwStart].DataSize;
		lpData = lpEvents[lpFilters[i].qwStart].Data.p;
		Long = (qwFilter>=FILTER_SEQUENCENUMBER) && (qwFilter<=FILTER_SYSTEMEXCLUSIVE);
		Mask = ILD_NORMAL;

		switch (qwFilter)
		{
		case FILTER_NOTES:
			Block1.top = Notes.top + (LastNote-Byte1) * KeyHeight;
			Block1.bottom = Block1.top + KeyHeight;
			if ((Block1.top>=Notes.top) && (Block1.bottom<=(Notes.bottom+4)))
			{
				DrawBlock1 = TRUE;
				Block1.left = Notes.left + (Start-CurrentPosition)*TickWidth;
				Block1.right = Notes.left + (End-CurrentPosition)*TickWidth;
			}
			if (Controller==-2)
			{
				DrawBlock2 = TRUE;
				Block2.top = Controllers.top + 127-Byte2;
				Block2.bottom = Controllers.bottom;
				Block2.left = Notes.left + (Start-CurrentPosition)*TickWidth;
				Block2.right = Block2.left + Beat * TickWidth / 16 + 1;
			}
			break;
		case FILTER_CONTROLLER:
			switch (Byte1)
			{
			case 0x00: BankHigh = Byte2; break;
			case 0x20: BankLow = Byte2; break;
			default: break;
			}
			if (Byte1==Controller)
			{
				DrawBlock2 = TRUE;
				Block2.top = Controllers.top + 127-Byte2;
				Block2.bottom = Controllers.bottom;
				Block2.left = Notes.left + (Start-CurrentPosition)*TickWidth;
				Block2.right = Block2.left + Beat * TickWidth / 16 + 1;
			}
			break;
		case FILTER_PITCHWHEEL:
			if (Controller==-1)
			{
				DrawBlock2 = TRUE;
				Block2.top = Controllers.top + 127 - (Byte1 | (Byte2 << 7)) * 127 / 0x3FFF;
				Block2.bottom = Controllers.bottom;
				Block2.left = Notes.left + (Start-CurrentPosition)*TickWidth;
				Block2.right = Block2.left + Beat * TickWidth / 16 + 1;
			}
			break;
		case FILTER_PROGRAM:
			SetTextColor(buffer, 0x000000);
			Icon = ID_PROGRAM;
			Long = TRUE;
			GetInstrumentName(BankHigh, BankLow, Byte1, lpstr1);
			if (!strlen(lpstr1))
			{
				strcpy(lpstr1, Strings[39]);
				strcat(lpstr1, Strings[16]);
				if (BankLow==-1) j=BankHigh; else j=BankLow;
				strcat(lpstr1, itoa(j, lpstr2, 10));
				strcat(lpstr1, Strings[40]);
				strcat(lpstr1, itoa(Byte1, lpstr2, 10));
			}
			if ((Meta==ID_PROGRAM) && (Start==Selection.right)) Mask = ILD_SELECTED;
			break;
		case FILTER_TEMPO:
			SetTextColor(buffer, 0x000000);
			Icon = ID_TEMPO;
			Long = TRUE;
			j = (lpData[0] << 16) | (lpData[1] << 8) | lpData[2];
			itoa(60000000 / j, lpstr1, 10);
			strcat(lpstr1, Strings[41]);
			if ((Meta==ID_TEMPO) && (Start==Selection.right)) Mask = ILD_SELECTED;
			break;
		case FILTER_TEXT:
			SetTextColor(buffer, 0x0000FF);
			Icon = ID_TEXT;
			Long = TRUE;
			strncat(lpstr1, (const char*) lpData, DataSize);
			if ((Meta==ID_TEXT) && (Start==Selection.right)) Mask = ILD_SELECTED;
			break;
		case FILTER_LYRIC:
			SetTextColor(buffer, 0x008000);
			Icon = ID_LYRIC;
			Long = TRUE;
			strncat(lpstr1, (const char*) lpData, DataSize);
			if ((Meta==ID_LYRIC) && (Start==Selection.right)) Mask = ILD_SELECTED;
			break;
		case FILTER_MARKER:
			SetTextColor(buffer, 0xFF0000);
			Icon = ID_MARKER;
			Long = TRUE;
			strncat(lpstr1, (const char*) lpData, DataSize);
			if ((Meta==ID_MARKER) && (Start==Selection.right)) Mask = ILD_SELECTED;
			break;
		case FILTER_CUEPOINT:
			SetTextColor(buffer, 0x008080);
			Icon = ID_CUEPOINT;
			Long = TRUE;
			strncat(lpstr1, (const char*) lpData, DataSize);
			if ((Meta==ID_CUEPOINT) && (Start==Selection.right)) Mask = ILD_SELECTED;
			break;
		case FILTER_TRACKNAME: /* Just ignores */
		case FILTER_COPYRIGHT:
		case FILTER_SEQUENCENUMBER:
		case FILTER_INSTRUMENTNAME:
		case FILTER_ENDOFTRACK:
		case FILTER_NOTEON:
		case FILTER_NOTEOFF:
			continue;
		default: /* Error message */
			SetTextColor(buffer, 0x800080);
			strcpy(lpstr1, Strings[42]);
			if (Event<16) strcat(lpstr1, "0");
			strcat(lpstr1, itoa(Event,lpstr2, 16));
			strcat(lpstr1, "): ");
			if (Long)
				for (j=0; j<DataSize; j++)
				{
					strcat(lpstr1, "0x");
					if (lpData[j] < 16) strcat(lpstr1, "0");
					strcat(lpstr1, itoa(lpData[j], lpstr2, 16));
					strcat(lpstr1, " ");
				}
			else
			{
				strcat(lpstr1, "0x");
				if (Byte1 < 16) strcat(lpstr1, "0");
				strcat(lpstr1, itoa(Byte1, lpstr2, 16));
				strcat(lpstr1, " 0x");
				if (Byte2 < 16) strcat(lpstr1, "0");
				strcat(lpstr1, itoa(Byte2, lpstr2, 16));
			}
			Icon = ID_UNKNOWN;
			Long = TRUE;
			if ((Meta==ID_UNKNOWN) && (Start==Selection.right)) Mask = ILD_SELECTED;
			break;
		}

		if (Long)
		{
			GetTextExtentPoint32(buffer, lpstr1, strlen(lpstr1), &size);
			Length = 1 + size.cx / TickWidth;
		}
		else Length = lpEvents[lpFilters[i].qwEnd].dwTicks - lpEvents[lpFilters[i].qwStart].dwTicks;
		End = Start + Length;
		if (End<(CurrentPosition-Interval)) continue;
		if (Long)
		{
			if (Start<Last) Rows++; else Rows = 0;
			if (End > Last) Last = End;
			if ((Notes.top + Rows*11)<Notes.bottom)
				TextOut(buffer, Notes.left + (Start-CurrentPosition)*TickWidth, Notes.top + Rows*11, lpstr1, strlen(lpstr1));
		}
		if (DrawBlock1)
		{
			top = Block1.top;
			bottom = Block1.bottom;
			left = Block1.left;
			right = Block1.right;
			SelectObject(buffer, blackp);
			if (Selected)
				SelectObject(buffer, red);
			else
				SelectObject(buffer, blue);
			Rectangle(buffer, left, top, right, bottom);

			if (Selected)
			{
				if ((Movement==RESIZING) && (Byte1==Selection.top) && ((Start==Selection.left) || (End==Selection.left)))
				{
					if (Start==Selection.left)
						left += OffsetX * TickWidth;
					if (End==Selection.left)
						right += OffsetX * TickWidth;
					if (((Start==Selection.left) && (Start>=-OffsetX)) || ((End==Selection.left) && (End>=-OffsetX)))
						SelectObject(buffer, dottedp);
					else
						SelectObject(buffer, redp);
					SelectObject(buffer, hollow);
					if ((top>=Notes.top) && (bottom<=Notes.bottom))
						Rectangle(buffer, left, top-1, right, bottom+1);
				}
				if (Movement==TRANSPOSING)
				{
					left += OffsetX*TickWidth;
					right += OffsetX*TickWidth;
					top -= OffsetY*KeyHeight;
					bottom -= OffsetY*KeyHeight;
					if (((Start+OffsetX)>=0) && ((End+OffsetX)>=0) && ((Byte1+OffsetY)>=0) && ((Byte1+OffsetY)<=127))
						SelectObject(buffer, dottedp);
					else
						SelectObject(buffer, redp);
					SelectObject(buffer, hollow);
					if ((top>=Notes.top) && (bottom<=Notes.bottom))
						Rectangle(buffer, left, top, right, bottom);
				}
			}
		}
		if (DrawBlock2)
		{
			SelectObject(buffer, blackp);
			if (Start == Selection.right)
			{
				SelectObject(buffer, red);
				if (CurrentController==-1) CurrentController = Byte2;
			}
			else SelectObject(buffer, blue);
			Rectangle(buffer, Block2.left, Block2.top, Block2.right, Block2.bottom);
		}
		if (Icon!=-1)
			if ((Start!=Selection.left) || (Icon!=Meta) || (Movement!=DRAGGING))
				ImageList_Draw(imagelist, Icon, buffer, Notes.left + (Start-CurrentPosition)*TickWidth - 4, Bar.top+2, Mask);
	}

	MidiCleanEvents(nEvents, lpEvents);
	MidiFreeBuffer(lpFilters);

	/* Marks current playback position */
	i = Notes.left + (Ticks - CurrentPosition)*TickWidth;
	if ((i>=Notes.left) && (i<=Notes.right))
	{
		SelectObject(buffer, redp);
		MoveToEx(buffer, i, Notes.top + 1, NULL);
		LineTo(buffer, i, Notes.bottom - 1);
	}

	/* Marks current mouse position */
	if (((Selection.right-CurrentPosition)*TickWidth)>=0)
	{
		SelectObject(buffer, greenp);
		MoveToEx(buffer, Notes.left + (Selection.right-CurrentPosition)*TickWidth, Notes.top + 1, NULL);
		LineTo(buffer, Notes.left + (Selection.right-CurrentPosition)*TickWidth, Notes.bottom - 1);
	}

	/* Selection marker */
	if (Movement==SELECTING)
	{
		SelectObject(buffer, dottedp);
		SelectObject(buffer, hollow);
		left   = Notes.left + (Selection.left - CurrentPosition)*TickWidth,
		top    = Notes.top + (LastNote - Selection.top) * KeyHeight;
		right  = Notes.left + (Selection.right - CurrentPosition)*TickWidth,
		bottom = Notes.top + (LastNote - Selection.bottom) * KeyHeight;
		if (top>bottom) { i = top; top = bottom; bottom = i; }
		if (left>right) { i = left; left = right; right = i; }
		bottom += KeyHeight;
		if (top<=Notes.top) top = Notes.top;
		if (bottom>=Notes.bottom) bottom = Notes.bottom;
		if (left<=Notes.left) left = Notes.left - 1;
		if (right>=Notes.right) right = Notes.right;
		Rectangle(buffer, left, top, right, bottom);
	}

	/* Note shadow */
	if (Mode!=ID_SELECT)
	{
		SelectObject(buffer, dottedp);
		SelectObject(buffer, hollow);
		left = Notes.left + (Selection.right-CurrentPosition)*TickWidth;
		top = Notes.top + (LastNote - Selection.bottom) * KeyHeight;
		right = left + ((Beat << 3) >> (Mode-ID_0)) * TickWidth;
		bottom = top + KeyHeight;
		if ((top>=Notes.top) && (bottom<=Notes.bottom))
			Rectangle(buffer, left, top, right, bottom);
	}

	/* Piano background */
	SelectObject(buffer, nullp);
	SelectObject(buffer, GetSysColorBrush(COLOR_3DFACE));
	Rectangle(buffer, Piano.left-1, Piano.top, Piano.right+1, Piano.bottom);
	SelectObject(buffer, blackp);

	/* Piano keys */
	SetTextColor(buffer, 0x0000FF);
	for (i=FirstNote; i<=LastNote; i++)
	if ((i>=0) && (i<=127))
	{
		Key = i % 12;
		if ((Key==1) || (Key==3) || (Key==6) || (Key==8) || (Key==10))
			SelectObject(buffer, black);
		else
			SelectObject(buffer, white);
		Rectangle(buffer, Piano.left, Piano.top + (LastNote-i)*KeyHeight, Piano.right, Piano.top + (LastNote-i+1)*KeyHeight + 1);
		if (Key==0)
		{
			KeyText(i, lpstr1);
			TextOut(buffer, 50, Piano.top + (LastNote-i)*KeyHeight, lpstr1, strlen(lpstr1));
		}
	}

	/* Current note */
	if ((Selection.bottom>=0) && (Selection.bottom<=127) && ((LastNote-Selection.bottom)>=0))
	{
		KeyText(Selection.bottom, lpstr1);
		TextOut(buffer, 50, Piano.top + (LastNote - Selection.bottom) *KeyHeight, lpstr1, strlen(lpstr1));
	}


	BitBlt(hdc, Piano.left, Ruler.top, Ruler.right - Ruler.left, Controllers.bottom-Ruler.top, buffer, Piano.left, Ruler.top, SRCCOPY);

	/* Controller tool-tip */
	if (CurrentController!=-1)
	{
		itoa(CurrentController, lpstr1,10);
		GetTextExtentPoint32(hdc, lpstr1, strlen(lpstr1), &size);
		SelectObject(hdc, GetSysColorBrush(COLOR_INFOBK));
		SelectObject(hdc, GetStockObject(BLACK_PEN));
		if ((MouseY-size.cy-4)>Controllers.top)
		{
			Rectangle(hdc, MouseX-size.cx-8, MouseY-size.cy-4, MouseX, MouseY);
			TextOut(hdc, MouseX-size.cx-4, MouseY-size.cy-2, lpstr1, strlen(lpstr1));
		}
		else
		{
			Rectangle(hdc, MouseX-size.cx-8, Controllers.top, MouseX, Controllers.top+size.cy+4);
			TextOut(hdc, MouseX-size.cx-4, Controllers.top+2, lpstr1, strlen(lpstr1));
		}
	}

	AdjustTitleBar();
}

void AdjustWindow(void)
{
	int i, Channel, BankHigh, BankLow, Program, cx;
	SIZE size;

	Movement = 0;

	AdjustScroll();

	cx = 0;
	SendMessage(GetDlgItem(window, ID_TRACK), CB_RESETCONTENT, 0, 0);
	for (i=0; i<(int) MidiGet(Sequence, TRACK_COUNT); i++)
	{
		GetProgramInformation(i, -1, &Channel, &BankHigh, &BankLow, &Program);
		MetaAction(META_SELECT, i, -1, FILTER_TRACKNAME, lpstr1, 256);
		if (Channel!=-1)
		{
			strcat(lpstr1, " (");
			GetInstrumentName(BankHigh, BankLow, Program, lpstr2);
			if (!strlen(lpstr2)) strcat(strcpy(lpstr2, Strings[9]), itoa(Channel, lpstr3, 10));
			strcat(lpstr1, lpstr2);
			strcat(lpstr1, ")");
		}
		SendMessage(GetDlgItem(window, ID_TRACK), CB_ADDSTRING, 0, (LPARAM) lpstr1);
		GetTextExtentPoint32(GetDC(GetDlgItem(window, ID_TRACK)), lpstr1, strlen(lpstr1), &size);
		if (size.cx > cx) cx = size.cx;
	}

	if (Beat % 16)
		SetWindowText(GetDlgItem(window, ID_BEAT), strcat(itoa(Beat, lpstr1,10), Strings[43]));
	else
		SendMessage(GetDlgItem(window, ID_BEAT), CB_SETCURSEL, (Beat >> 4) - 1, 0);

	SendMessage(GetDlgItem(window, ID_TRACK), CB_SETDROPPEDWIDTH, cx, 0);
	SendMessage(GetDlgItem(window, ID_TRACK), CB_SETCURSEL, Track, 0);
	SendMessage(GetDlgItem(window, ID_DEVICE), CB_SETCURSEL, Device+1, 0);
	SendMessage(GetDlgItem(window, ID_ZOOM), CB_SETCURSEL, Zoom / 5 - 1, 0);
	SendMessage(GetDlgItem(window, ID_CONTROLLER), CB_SETCURSEL, Controller+2, 0);


	AdjustButtons();

	AdjustTitleBar();

	SetFocus(window);

	Redraw();
}

void AdjustButtons(void)
{
	int i;

	for (i=ID_SELECT; i<=ID_64; i++) FlatButton(i, Mode == i);
	for (i=ID_PROGRAM; i<=ID_UNKNOWN; i++) FlatButton(i, Meta == i);

	FlatButton(ID_MUTE, (BOOL) MidiTrackGet(Sequence, Track, TRACK_MUTE));
	FlatButton(ID_SOLO, ((int) MidiGet(Sequence, SOLO_TRACK)) == Track);
	FlatButton(ID_LOOP, (BOOL) MidiGet(Sequence, MIDI_LOOP));
	FlatButton(ID_PLAY, MidiGet(Sequence, MIDI_STATUS)==MIDI_PLAY);
	FlatButton(ID_PAUSE, MidiGet(Sequence, MIDI_STATUS)==MIDI_PAUSE);
	FlatButton(ID_STOP, MidiGet(Sequence, MIDI_STATUS)==MIDI_STOP);
}


void AdjustTrackChannels(WORD wTrack)
{
	int i, j, CurrentChannel = 0, type, nEvents, nFilters;
	QWORD filter;
	LPBRELS_MIDI_EVENT lpEvents;
	LPBRELS_FILTERED_EVENT lpFilters;

	UNREFERENCED_PARAMETER(wTrack);

	nEvents = MidiTrackGet(Sequence, Track, EVENT_COUNT);
	MidiGetTrackEvents(Sequence, Track, 0, nEvents-1, MIDI_INDEX, &lpEvents);
	nFilters = MidiFilterTrackEvents(Sequence, Track, 0, nEvents-1, MIDI_INDEX, FILTER_ALL | FILTER_NOTES, &lpFilters);

	for (i=0; i<nFilters; i++)
		{
			type = lpEvents[lpFilters[i].qwStart].Event & 0xF0;
			filter = lpFilters[i].qwFilter;

			if ((filter==FILTER_NOTES) || (filter==FILTER_KEYAFTERTOUCH) || (filter==FILTER_CONTROLLER) ||
				(filter==FILTER_PROGRAM) || (filter==FILTER_CHANNELPRESSURE) || (filter==FILTER_PITCHWHEEL) )
			{
				if (type==0xC0)
				{
					CurrentChannel = lpEvents[lpFilters[i].qwStart].Event & 0x0F;
					/* Sets earlier events on the same tick position */
					for (j=i-1; j>=0; j--)
						if (lpEvents[lpFilters[j].qwStart].dwTicks==lpEvents[lpFilters[i].qwStart].dwTicks)
						{
							lpEvents[lpFilters[j].qwStart].Event = (lpEvents[lpFilters[j].qwStart].Event & 0xF0) | CurrentChannel;
							if (lpFilters[j].qwEnd!=1) lpEvents[lpFilters[j].qwEnd].Event = (lpEvents[lpFilters[j].qwEnd].Event & 0xF0) | CurrentChannel;
						}
						else
							break;
				}
				lpEvents[lpFilters[i].qwStart].Event = type | CurrentChannel;
				if (((int) lpFilters[i].qwEnd)!=-1) lpEvents[lpFilters[i].qwEnd].Event = type | CurrentChannel;
			}
		}

	MidiSuspend(Sequence, TRUE);
	MidiSilence(Sequence);
	MidiRemoveTrack(Sequence, Track);
	MidiInsertTrack(Sequence, Track);
	MidiInsertTrackEvents(Sequence, Track, nEvents, lpEvents);
	MidiResume(Sequence, TRUE);
	MidiCleanEvents(nEvents, lpEvents);
	MidiFreeBuffer(lpFilters);

}

void AdjustScroll(void)
{
	SCROLLINFO si;

	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_RANGE | SIF_PAGE;
	si.nMin = 0;
	si.nMax = 127;
	si.nPage = 1;
	SetScrollInfo(GetDlgItem(window, ID_NOTE), SB_CTL, &si, TRUE);
	si.nMin = 0;
	si.nMax = MidiGet(Sequence, TICK_COUNT);
	SetScrollInfo(GetDlgItem(window, ID_POSITION), SB_CTL, &si, TRUE);
}

void AdjustTitleBar(void)
{
	QWORD Time, MaxTime;
	DWORD Ticks, MaxTicks, Min, Sec, Mil;

	strcpy(lpstr1, Strings[38]);
	strcat(lpstr1, " [");
	strcpy(lpstr2, "");
	GetFileTitle(lpfile, lpstr2, 256);
	if (strlen(lpstr2)==0) strcpy(lpstr2, Strings[57]);
	strcat(lpstr1, lpstr2);
	if (!Saved) strcat(lpstr1, Strings[44]);
	strcat(lpstr1, "] - ");

	if (MidiGet(Sequence, MIDI_STATUS)==MIDI_PLAY)
		Time = MidiGet(Sequence, CURRENT_TIME);
	else
		if (Selection.right>=0)
			Time = MidiQuery(Sequence, 0, MIDI_TIME, MIDI_TICKS, Selection.right);
		else
			Time = 0;
	Min = Time / 60000000;
	Sec = (Time / 1000000) % 60;
	Mil = (Time % 1000000) / 1000;
	if (Min<10) strcat(lpstr1, "0");
	strcat(lpstr1, itoa(Min, lpstr2, 10));
	strcat(lpstr1, ":");
	if (Sec<10) strcat(lpstr1, "0");
	strcat(lpstr1, itoa(Sec, lpstr2, 10));
	strcat(lpstr1, ":");
	if (Mil<100) strcat(lpstr1, "0");
	if (Mil< 10) strcat(lpstr1, "0");
	strcat(lpstr1, itoa(Mil, lpstr2, 10));
	strcat(lpstr1, "/");

	MaxTime = MidiGet(Sequence, TIME_COUNT);
	Min = MaxTime / 60000000;
	Sec = (MaxTime / 1000000) % 60;
	Mil = (MaxTime % 1000000) / 1000;
	if (Min<10) strcat(lpstr1, "0");
	strcat(lpstr1, itoa(Min, lpstr2, 10));
	strcat(lpstr1, ":");
	if (Sec<10) strcat(lpstr1, "0");
	strcat(lpstr1, itoa(Sec, lpstr2, 10));
	strcat(lpstr1, ":");
	if (Mil<100) strcat(lpstr1, "0");
	if (Mil< 10) strcat(lpstr1, "0");
	strcat(lpstr1, itoa(Mil, lpstr2, 10));
	strcat(lpstr1, " - ");

	if (MidiGet(Sequence, MIDI_STATUS)==MIDI_PLAY)
		Ticks = MidiGet(Sequence, CURRENT_TICKS);
	else
		if (Selection.right>=0)
			Ticks = Selection.right;
		else
			Ticks = 0;
	strcat(lpstr1, itoa(Ticks, lpstr2, 10));
	strcat(lpstr1, "/");

	MaxTicks = MidiGet(Sequence, TICK_COUNT);
	strcat(lpstr1, itoa(MaxTicks, lpstr2, 10));
	strcat(lpstr1, Strings[43]);
	SetWindowText(window, lpstr1);
}

void FlatButton(int ID, BOOL Flat)
{
	SendMessage(GetDlgItem(window, ID), BM_SETSTATE, Flat, 0);
}

BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam)
{
	HDC childDC;

	UNREFERENCED_PARAMETER(lParam);

	SendMessage(hwnd, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

	childDC = GetDC(hwnd);
	SelectObject(childDC, GetStockObject(DEFAULT_GUI_FONT));
	ReleaseDC(hwnd, childDC);

	return TRUE;
}

LRESULT CALLBACK GroupBoxProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
	case WM_NOTIFY:
		SendMessage(GetParent(hwnd), uMsg, wParam, lParam);
		break;
	default: break;
	}

	return CallWindowProc((WNDPROC) GetWindowLongPtr(hwnd, GWLP_USERDATA), hwnd, uMsg, wParam, lParam);
}

void FilterSequence(void)
{
	HWND filter;
	QWORD qwLength;
	int i, j, min, sec, ms;

	EnableWindow(window, FALSE);

	qwLength = MidiGet(Sequence, TIME_COUNT);
	min = qwLength / 60000000;
	sec = (qwLength / 1000000) % 60;
	ms = (qwLength % 1000000) / 1000;

	filter = CreateWindow("FILTERWINDOW", Strings[58], WS_CAPTION | WS_SYSMENU | WS_POPUP, GetSystemMetrics(SM_CXSCREEN) / 2 - 200, GetSystemMetrics(SM_CYSCREEN) / 2 - 176, 400 + GetSystemMetrics(SM_CXFIXEDFRAME)*2, 352+GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFIXEDFRAME)*2, window, NULL, wc.hInstance, NULL);

	CreateWindowEx(WS_EX_CONTROLPARENT, "BUTTON", Strings[59], WS_CHILD | WS_VISIBLE | BS_GROUPBOX | WS_TABSTOP, 4, 4, 392, 48, filter, (HMENU) 0xFFF0, wc.hInstance, NULL);
	ZeroMemory(lpstr1, MAX_PATH);
	if (!strlen(lpfile)) strcpy(lpfile, Strings[57]);
	j = strlen(lpfile)-1;
	for (i=0; i<(int) strlen(lpfile); i++) if (lpfile[i]=='.') j=i;
	strncpy(lpstr1, lpfile, j);
	strcat(lpstr1, Strings[83]);
	if (j!=(((int) strlen(lpfile))-1)) strcat(lpstr1, &lpfile[j]);
	CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", lpstr1, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 8, 20, 308, 20, GetDlgItem(filter, 0xFFF0), (HMENU) 0xFFF1, wc.hInstance, NULL);
	CreateWindow("BUTTON", Strings[60], WS_CHILD | WS_VISIBLE | WS_TABSTOP, 320, 16, 64, 24, GetDlgItem(filter, 0xFFF0), (HMENU) 0xFFF2, wc.hInstance, NULL);
	SetWindowLongPtr(GetDlgItem(filter, 0xFFF0), GWLP_USERDATA, SetWindowLongPtr(GetDlgItem(filter, 0xFFF0), GWLP_WNDPROC, (LONG_PTR) GroupBoxProc));

	CreateWindowEx(WS_EX_CONTROLPARENT, "BUTTON", Strings[61], WS_CHILD | WS_VISIBLE | BS_GROUPBOX | WS_TABSTOP, 4, 56, 392, 60, filter, (HMENU) 0xFFF3, wc.hInstance, NULL);
	CreateWindow("STATIC", Strings[62], WS_CHILD | WS_VISIBLE, 8, 16, 32, 16, GetDlgItem(filter, 0xFFF3), NULL, wc.hInstance, NULL);
	CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "00", WS_TABSTOP | ES_NUMBER | WS_CHILD | WS_VISIBLE, 8, 32, 20, 20, GetDlgItem(filter, 0xFFF3), (HMENU) 0xFFF4, wc.hInstance, NULL);
	CreateWindow("STATIC", ":", WS_CHILD | WS_VISIBLE, 32, 36, 4, 16, GetDlgItem(filter, 0xFFF3), NULL, wc.hInstance, NULL);
	CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "00", WS_TABSTOP | ES_NUMBER | WS_CHILD | WS_VISIBLE, 40, 32, 20, 20, GetDlgItem(filter, 0xFFF3), (HMENU) 0xFFF5, wc.hInstance, NULL);
	CreateWindow("STATIC", ":", WS_CHILD | WS_VISIBLE, 64, 36, 4, 16, GetDlgItem(filter, 0xFFF3), NULL, wc.hInstance, NULL);
	CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "000", WS_TABSTOP | ES_NUMBER | WS_CHILD | WS_VISIBLE, 72, 32, 28, 20, GetDlgItem(filter, 0xFFF3), (HMENU) 0xFFF6, wc.hInstance, NULL);
	CreateWindow("STATIC", Strings[63], WS_CHILD | WS_VISIBLE, 136, 16, 32, 16, GetDlgItem(filter, 0xFFF3), NULL, wc.hInstance, NULL);
	if (min < 10) strcpy(lpstr1, "0"); else strcpy(lpstr1, "");
	strcat(lpstr1, itoa(min, lpstr2, 10));
	CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", lpstr1, WS_TABSTOP | ES_NUMBER | WS_CHILD | WS_VISIBLE, 136, 32, 20, 20, GetDlgItem(filter, 0xFFF3), (HMENU) 0xFFF7, wc.hInstance, NULL);
	CreateWindow("STATIC", ":", WS_CHILD | WS_VISIBLE, 160, 36, 4, 16, GetDlgItem(filter, 0xFFF3), NULL, wc.hInstance, NULL);
	if (sec < 10) strcpy(lpstr1, "0"); else strcpy(lpstr1, "");
	strcat(lpstr1, itoa(sec, lpstr2, 10));
	CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", lpstr1, WS_TABSTOP | ES_NUMBER | WS_CHILD | WS_VISIBLE, 168, 32, 20, 20, GetDlgItem(filter, 0xFFF3), (HMENU) 0xFFF8, wc.hInstance, NULL);
	CreateWindow("STATIC", ":", WS_CHILD | WS_VISIBLE, 192, 36, 4, 16, GetDlgItem(filter, 0xFFF3), NULL, wc.hInstance, NULL);
	if (ms < 10) strcpy(lpstr1, "0"); else strcpy(lpstr1, "");
	if (ms < 100) strcat(lpstr1, "0");
	strcat(lpstr1, itoa(ms, lpstr2, 10));
	CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", lpstr1, WS_TABSTOP | ES_NUMBER | WS_CHILD | WS_VISIBLE, 200, 32, 28, 20, GetDlgItem(filter, 0xFFF3), (HMENU) 0xFFF9, wc.hInstance, NULL);
	CreateWindow("BUTTON", Strings[64], WS_TABSTOP | WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 264, 36, 120, 16, GetDlgItem(filter, 0xFFF3), (HMENU) 0xFFFA, wc.hInstance, NULL);

	CreateWindowEx(WS_EX_CONTROLPARENT, "BUTTON", Strings[66], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_GROUPBOX, 4, 120, 192, 200, filter, (HMENU) 0xFFFB, wc.hInstance, NULL);
	CreateWindowEx(WS_EX_CLIENTEDGE, "LISTBOX", "", WS_VSCROLL | WS_CHILD | WS_VISIBLE | WS_TABSTOP | LBS_HASSTRINGS | LBS_EXTENDEDSEL | LBS_NOINTEGRALHEIGHT, 8, 16, 176, 176, GetDlgItem(filter, 0xFFFB), (HMENU) 0xFFFC, wc.hInstance, NULL);
	for (i=69; i<83; i++)
		SendMessage(GetDlgItem(GetDlgItem(filter, 0xFFFB), 0xFFFC), LB_ADDSTRING, 0, (LPARAM) Strings[i]);

	CreateWindowEx(WS_EX_CONTROLPARENT, "BUTTON", Strings[65], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_GROUPBOX, 204, 120, 192, 200, filter, (HMENU) 0xFFFD, wc.hInstance, NULL);
	CreateWindowEx(WS_EX_CLIENTEDGE, "LISTBOX", "", WS_HSCROLL | WS_VSCROLL | WS_CHILD | WS_VISIBLE | WS_TABSTOP | LBS_HASSTRINGS | LBS_EXTENDEDSEL | LBS_NOINTEGRALHEIGHT, 8, 16, 176, 176, GetDlgItem(filter, 0xFFFD), (HMENU) 0xFFFE, wc.hInstance, NULL);
	SendMessage(GetDlgItem(GetDlgItem(filter, 0xFFFD), 0xFFFE), LB_SETHORIZONTALEXTENT, 1024, 0);
	for (i=0; i < (int) MidiGet(Sequence, TRACK_COUNT); i++)
	{
		MetaAction(META_SELECT, i, -1, FILTER_TRACKNAME, lpstr1, 256);
		if (i<9) strcpy(lpstr2, "0"); else strcpy(lpstr2, "");
		strcat(lpstr2, itoa(i+1, lpstr3, 10));
		strcat(lpstr2, " - ");
		strcat(lpstr2, lpstr1);
		SendMessage(GetDlgItem(GetDlgItem(filter, 0xFFFD), 0xFFFE), LB_ADDSTRING, 0, (LPARAM) lpstr2);
	}
	SendMessage(GetDlgItem(GetDlgItem(filter, 0xFFFD), 0xFFFE), LB_SETSEL, TRUE, -1);

	SendMessage(GetDlgItem(GetDlgItem(filter, 0xFFFB), 0xFFFC), LB_SETSEL, TRUE, -1);
	SendMessage(GetDlgItem(GetDlgItem(filter, 0xFFFB), 0xFFFC), LB_SETSEL, FALSE, 13);

	CreateWindow("BUTTON", Strings[86], WS_CHILD | WS_TABSTOP | WS_VISIBLE | BS_AUTOCHECKBOX, 4, 328, 240, 16, filter, (HMENU) 0xFFFF, wc.hInstance, NULL);
	CreateWindow("BUTTON", Strings[14], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 256, 324, 68, 24, filter, (HMENU) IDOK, wc.hInstance, NULL);
	CreateWindow("BUTTON", Strings[15], WS_CHILD | WS_VISIBLE | WS_TABSTOP, 328, 324, 68, 24, filter, (HMENU) IDCANCEL, wc.hInstance, NULL);

	EnumChildWindows(filter, EnumChildProc, 0);

	ShowWindow(filter, SW_SHOW);

	while (IsWindow(filter))
	if (GetMessage(&msg, NULL, 0, 0)>0)
	if (!IsDialogMessage(GetActiveWindow(), &msg))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void FilterTrack(HANDLE NewHandle, WORD wSrcTrack, WORD wDstTrack, QWORD qwBase, QWORD qwStart, QWORD qwEnd, QWORD qwFilter, BOOL RoundTicks)
{
	int i;
	double Factor;
	DWORD nEvents, nNew, nFilter;
	LPBRELS_MIDI_EVENT lpEvents, lpNew;
	LPBRELS_FILTERED_EVENT lpFilter;

	nFilter = MidiFilterTrackEvents(Sequence, wSrcTrack, qwStart, qwEnd, MIDI_TICKS, qwFilter, &lpFilter);
	if (!nFilter) return;
	nEvents = MidiTrackGet(Sequence, wSrcTrack, EVENT_COUNT);
	MidiGetTrackEvents(Sequence, wSrcTrack, 0, nEvents-1, MIDI_INDEX, &lpEvents);
	lpNew = (LPBRELS_MIDI_EVENT) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BRELS_MIDI_EVENT)*nEvents);

	Factor = 16.0l / MidiGet(Sequence, BEAT_SIZE);

	nNew = 0;
	for (i=0; i<(int) nFilter; i++)
	{
		lpNew[nNew]=lpEvents[lpFilter[i].qwStart];
		lpNew[nNew].dwTicks=lpNew[nNew].dwTicks-qwStart + qwBase;
		if (RoundTicks) lpNew[nNew].dwTicks = round(lpNew[nNew].dwTicks*Factor);
		nNew++;
		if (lpFilter[i].qwFilter == FILTER_NOTES)
		{
			/* Uses note-on with velocity zero to reduce the file size */
			lpNew[nNew]=lpNew[nNew-1];
			lpNew[nNew].dwTicks = lpEvents[lpFilter[i].qwEnd].dwTicks;
			lpNew[nNew].Data.a[1] = 0;
			if (lpNew[nNew].dwTicks > qwEnd) lpNew[nNew].dwTicks = qwEnd;
			lpNew[nNew].dwTicks=lpNew[nNew].dwTicks-qwStart + qwBase;
			if (RoundTicks) lpNew[nNew].dwTicks = round(lpNew[nNew].dwTicks*Factor);
			nNew++;
		}
	}


	MidiInsertTrackEvents(NewHandle, wDstTrack, nNew, lpNew);

	MidiFreeBuffer(lpFilter);
	MidiCleanEvents(nEvents, lpEvents);
	HeapFree(GetProcessHeap(), 0, lpNew);

}

BOOL DoFilterSequence(HWND hwnd)
{
	HANDLE newseq;
	WORD wBeat;
	QWORD qwStart, qwEnd, qwFilter, min1, sec1, ms1, min2, sec2, ms2;
	HWND list1, list2;
	int i, nTracks;
	BOOL exclude, keep;

	GetWindowText(GetDlgItem(GetDlgItem(hwnd, 0xFFF3), 0xFFF4), lpstr1, MAX_PATH);
	min1 = atoi(lpstr1);

	GetWindowText(GetDlgItem(GetDlgItem(hwnd, 0xFFF3), 0xFFF5), lpstr1, MAX_PATH);
	sec1 = atoi(lpstr1);

	GetWindowText(GetDlgItem(GetDlgItem(hwnd, 0xFFF3), 0xFFF6), lpstr1, MAX_PATH);
	ms1 = atoi(lpstr1);

	GetWindowText(GetDlgItem(GetDlgItem(hwnd, 0xFFF3), 0xFFF7), lpstr1, MAX_PATH);
	min2 = atoi(lpstr1);

	GetWindowText(GetDlgItem(GetDlgItem(hwnd, 0xFFF3), 0xFFF8), lpstr1, MAX_PATH);
	sec2 = atoi(lpstr1);

	GetWindowText(GetDlgItem(GetDlgItem(hwnd, 0xFFF3), 0xFFF9), lpstr1, MAX_PATH);
	ms2 = atoi(lpstr1);

	exclude = SendMessage(GetDlgItem(GetDlgItem(hwnd, 0xFFF3), 0xFFFA), BM_GETCHECK, 0, 0);
	keep = SendMessage(GetDlgItem(hwnd, 0xFFFF), BM_GETCHECK, 0, 0);

	qwStart = min1 * 60000000 + sec1 * 1000000 + ms1 * 1000;
	qwEnd = min2 * 60000000 + sec2 * 1000000 + ms2 * 1000;
	qwStart = MidiQuery(Sequence, 0, MIDI_TICKS, MIDI_TIME, qwStart);
	qwEnd = MidiQuery(Sequence, 0, MIDI_TICKS, MIDI_TIME, qwEnd);

	if (((qwEnd-qwStart)>MidiGet(Sequence, TICK_COUNT)) || (qwStart>=qwEnd))
	{
		MessageBox(hwnd, Strings[87], Strings[85], MB_ICONWARNING);
		return FALSE;
	}

	GetWindowText(GetDlgItem(GetDlgItem(hwnd, 0xFFF0), 0xFFF1), lpstr2, MAX_PATH);
	if (PathFileExists(lpstr2))
		if (MessageBox(hwnd, Strings[84], Strings[85], MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING)==IDNO)
			return FALSE;

	nTracks = MidiGet(Sequence, TRACK_COUNT);
	wBeat = MidiGet(Sequence, BEAT_SIZE);
	if (!keep) wBeat = 16;

	qwFilter = 0;
	list1 = GetDlgItem(GetDlgItem(hwnd, 0xFFFB), 0xFFFC);
	list2 = GetDlgItem(GetDlgItem(hwnd, 0xFFFD), 0xFFFE);
	if (SendMessage(list1, LB_GETSEL,  0, 0)) qwFilter|=FILTER_COPYRIGHT;
	if (SendMessage(list1, LB_GETSEL,  1, 0)) qwFilter|=FILTER_TRACKNAME;
	if (SendMessage(list1, LB_GETSEL,  2, 0)) qwFilter|=FILTER_SEQUENCENUMBER;
	if (SendMessage(list1, LB_GETSEL,  3, 0)) qwFilter|=FILTER_INSTRUMENTNAME;
	if (SendMessage(list1, LB_GETSEL,  4, 0)) qwFilter|=FILTER_NOTES;
	if (SendMessage(list1, LB_GETSEL,  5, 0)) qwFilter|=FILTER_PROGRAM;
	if (SendMessage(list1, LB_GETSEL,  6, 0)) qwFilter|=FILTER_TEMPO;
	if (SendMessage(list1, LB_GETSEL,  7, 0)) qwFilter|=FILTER_TEXT;
	if (SendMessage(list1, LB_GETSEL,  8, 0)) qwFilter|=FILTER_LYRIC;
	if (SendMessage(list1, LB_GETSEL,  9, 0)) qwFilter|=FILTER_MARKER;
	if (SendMessage(list1, LB_GETSEL, 10, 0)) qwFilter|=FILTER_CUEPOINT;
	if (SendMessage(list1, LB_GETSEL, 11, 0)) qwFilter|=FILTER_PITCHWHEEL;
	if (SendMessage(list1, LB_GETSEL, 12, 0)) qwFilter|=FILTER_CONTROLLER;
	if (SendMessage(list1, LB_GETSEL, 13, 0)) qwFilter|=FILTER_CHANNELPREFIX | FILTER_SMPTEOFFSET | FILTER_TIMESIGNATURE | FILTER_KEYSIGNATURE | FILTER_SEQUENCERSPECIFIC | FILTER_SYSTEMEXCLUSIVE | FILTER_KEYAFTERTOUCH | FILTER_CHANNELPRESSURE | FILTER_MTCQUARTERFRAME | FILTER_SONGPOSITIONPOINTER | FILTER_SONGSELECT | FILTER_TUNEREQUEST /*| FILTER_MIDICLOCK | FILTER_MIDISTART | FILTER_MIDICONTINUE | FILTER_MIDISTOP*/ | FILTER_ACTIVESENSE;

	MidiCreate(NO_DEVICE, wBeat, &newseq);

	for (i=0; i<nTracks; i++)
	if (SendMessage(list2, LB_GETSEL, i, 0))
	{
		MidiInsertTrack(newseq, MidiGet(newseq, TRACK_COUNT));

		if (exclude)
		{
			FilterTrack(newseq, i, MidiGet(newseq, TRACK_COUNT)-1, 0, 0, qwStart, qwFilter, !keep);
			FilterTrack(newseq, i, MidiGet(newseq, TRACK_COUNT)-1, qwStart, qwEnd, MidiGet(Sequence, TICK_COUNT), qwFilter, !keep);
		}
		else FilterTrack(newseq, i, MidiGet(newseq, TRACK_COUNT)-1, 0, qwStart, qwEnd, qwFilter, !keep);
	}

	MidiEncode(newseq, lpstr2, TRUE);

	MidiClose(newseq);

	return TRUE;
}

LRESULT CALLBACK FilterProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	OPENFILENAME ofn;

	switch (uMsg)
	{
	case WM_CLOSE:
		EnableWindow(window, TRUE);
		DestroyWindow(hwnd);
		SetFocus(window);
		return 0;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			if (DoFilterSequence(hwnd))
				SendMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		case IDCANCEL:
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		case 0xFFF2:
			GetWindowText(GetDlgItem(GetDlgItem(hwnd, 0xFFF0), 0xFFF1), lpfile, MAX_PATH);
			ZeroMemory(&ofn, sizeof(OPENFILENAME));
			ofn.lStructSize = sizeof (OPENFILENAME); /*OPENFILENAME_SIZE_VERSION_400 ; */
			ofn.hwndOwner = hwnd;
			ofn.lpstrFilter = Strings[33];
			ofn.lpstrFile = lpfile;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
			ofn.lpstrDefExt = Strings[34];
			if (GetSaveFileName(&ofn))
				SetWindowText(GetDlgItem(GetDlgItem(hwnd, 0xFFF0), 0xFFF1), lpfile);
			break;
		default:
			break;
		}
		return 0;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int i;
	DWORD tickCount;

	switch (uMsg)
	{
	case WM_REDRAW:
		DoRedraw();
		return 0;
	case WM_DESTROY:
		MidiClose(Sequence);
		PostQuitMessage(0);
		return 0;
	case WM_CLOSE:
		if (CheckSave(Strings[45])) DestroyWindow(hwnd);
		return 0;
	case WM_PAINT:
		WindowPaint(wParam, lParam);
		return 0;
	case WM_KEYUP:
		KeyUp(wParam, lParam);
		return 0;
	case WM_KEYDOWN:
		KeyDown(wParam, lParam);
		return 0;
	case WM_COMMAND:
		if (LOWORD(wParam)<ID_HELP) SetFocus(window);

		switch (LOWORD(wParam))
		{
		case ID_FILENEW:  FileNew(); break;
		case ID_FILEOPEN: FileOpen(); break;
		case ID_FILESAVE: FileSave(lpfile); break;
		case ID_FILESAVEAS: FileSave(NULL); break;
		case ID_FILTER: FilterSequence(); break;
		case ID_DECODE: DecoderDialog(hwnd, Sequence); break;
		case ID_PLAY: MidiPlay(Sequence); AdjustButtons(); break;
		case ID_HOME:
			MidiReset(Sequence);
			CurrentPosition = 0;
			SetScrollPos(GetDlgItem(hwnd, ID_POSITION), SB_CTL, CurrentPosition, TRUE);
			break;
		case ID_PAUSE: MidiPause(Sequence); AdjustButtons(); break;
		case ID_END:
			MidiSuspend(Sequence, TRUE);
			MidiSilence(Sequence);
			tickCount = MidiGet(Sequence, TICK_COUNT);
			CurrentPosition = tickCount - PageSize;
			if (CurrentPosition<0) CurrentPosition = 0;
			MidiSet(Sequence, CURRENT_TICKS, CurrentPosition);
			SetScrollPos(GetDlgItem(hwnd, ID_POSITION), SB_CTL, CurrentPosition, TRUE);
			MidiResume(Sequence, TRUE);
			break;
		case ID_STOP: MidiStop(Sequence); AdjustButtons(); break;
		case ID_MUTE: MidiTrackSet(Sequence, Track, TRACK_MUTE, !MidiTrackGet(Sequence, Track, TRACK_MUTE)); AdjustButtons(); break;
		case ID_SOLO:
			if (((int) MidiGet(Sequence, SOLO_TRACK)) == Track)
				MidiSet(Sequence, SOLO_TRACK, NO_SOLO);
			else
				MidiSet(Sequence, SOLO_TRACK, Track);
			AdjustButtons();
			break;
		case ID_LOOP: MidiSet(Sequence, MIDI_LOOP, !MidiGet(Sequence, MIDI_LOOP)); AdjustButtons(); break;
		case ID_INSERTTRACK: InsertTrack(); break;
		case ID_REMOVETRACK: RemoveTrack(); break;
		case ID_COPYRIGHT:
			MetaAction(META_SELECT, 0, -1, FILTER_COPYRIGHT, lpstr1, 256);
			if (InputBox(window, Strings[46], Strings[47], lpstr1) == IDOK)
			{
				MidiSuspend(Sequence, TRUE);
				MidiSilence(Sequence);
				MetaAction(META_UPDATE, 0, -1, FILTER_COPYRIGHT, lpstr1, strlen(lpstr1));
				MidiResume(Sequence, TRUE);
			}
			break;
		case ID_TRACKNAME:
			MetaAction(META_SELECT, Track, -1, FILTER_TRACKNAME, lpstr1, 256);
			if (InputBox(window, Strings[48], Strings[49], lpstr1) == IDOK)
			{
				MidiSuspend(Sequence, TRUE);
				MidiSilence(Sequence);
				MetaAction(META_UPDATE, Track, -1, FILTER_TRACKNAME, lpstr1, strlen(lpstr1));
				MidiResume(Sequence, TRUE);
				AdjustWindow();
			}
			break;
		case ID_TRACKNUMBER:
			i = -1;
			if (MetaAction(META_SELECT, Track, -1, FILTER_SEQUENCENUMBER, lpstr1, 256))
				i = (lpstr1[0] << 8) | (lpstr1[1]);
			itoa(i, lpstr1, 10);
			if (InputBox(window, Strings[50], Strings[51], lpstr1)==IDOK)
			{
				i = (WORD) atoi(lpstr1);
				if (i>=(int) MidiGet(Sequence, TRACK_COUNT))
				{
					MessageBox(window, Strings[52], Strings[29], MB_ICONINFORMATION);
					return 0;
				}
				MidiSuspend(Sequence, TRUE);
				MidiSilence(Sequence);
				MidiTrackSet(Sequence, Track, TRACK_INDEX, i);
				Track = i;
				RenumberTracks();
				MidiResume(Sequence, TRUE);
				AdjustWindow();
			}
			break;
		case ID_INSTRUMENTNAME:
			MetaAction(META_SELECT, Track, -1, FILTER_INSTRUMENTNAME, lpstr1, 256);
			if (InputBox(window, Strings[53], Strings[54], lpstr1) == IDOK)
			{
				MidiSuspend(Sequence, TRUE);
				MidiSilence(Sequence);
				MetaAction(META_UPDATE, Track, -1, FILTER_INSTRUMENTNAME, lpstr1, strlen(lpstr1));
				MidiResume(Sequence, TRUE);
				AdjustWindow();
			}
			break;
		case ID_HELP:
			switch (GetUserDefaultLangID())
			{
			default:
				ShellExecute(window, NULL, "help.html", NULL, NULL, SW_SHOW);
				break;
			}
			break;
		case ID_ABOUT: MessageBox(window, Strings[55], Strings[56], MB_OK | MB_ICONINFORMATION); break;
		case ID_API: ShellExecute(window, NULL, "documentation\\index.html", NULL, NULL, SW_SHOW); break;
		case ID_CUT: CopyEvents(); DeleteEvents(); break;
		case ID_COPY: CopyEvents(); break;
		case ID_PASTE: PasteEvents(); break;
		case ID_DELETE: DeleteEvents(); break;
		case ID_SELECTALL: SelectAllEvents(); break;
		case ID_TRANSPOSEUP  : Transpose(0, +1); break;
		case ID_TRANSPOSEDOWN: Transpose(0, -1); break;
		case ID_OCTAVEUP     : Transpose(0, +12); break;
		case ID_OCTAVEDOWN   : Transpose(0, -12); break;
		case ID_SELECT       :
		case ID_0            :
		case ID_1            :
		case ID_2            :
		case ID_4            :
		case ID_8            :
		case ID_16           :
		case ID_32           :
		case ID_64           :
			Mode = LOWORD(wParam);
			AdjustButtons();
			break;
		case ID_PROGRAM      :
		case ID_TEMPO        :
		case ID_TEXT         :
		case ID_LYRIC        :
		case ID_MARKER       :
		case ID_CUEPOINT     :
		case ID_UNKNOWN      :
			Meta = LOWORD(wParam);
			AdjustButtons();
			break;
		case ID_TRACK        : if (HIWORD(wParam)==CBN_SELCHANGE) Track = SendMessage((HWND) lParam, CB_GETCURSEL, 0, 0); AdjustButtons(); Redraw(); return 0;
		case ID_CONTROLLER   : if (HIWORD(wParam)==CBN_SELCHANGE) Controller = SendMessage((HWND) lParam, CB_GETCURSEL, 0, 0) - 2; Redraw(); return 0;
		case ID_ZOOM         : if (HIWORD(wParam)==CBN_SELCHANGE) Zoom = (SendMessage((HWND) lParam, CB_GETCURSEL, 0, 0) + 1) * 5; Redraw(); return 0;
		case ID_DEVICE       : if (HIWORD(wParam)==CBN_SELCHANGE) ChangeDevice(); Redraw(); return 0;
		case ID_BEAT         : if (HIWORD(wParam)==CBN_SELCHANGE) ChangeBeat(); Redraw(); return 0;
		default: return 0;
		}
		DoRedraw();
		return 0;
	case WM_SIZE:
		WindowSize(wParam, lParam);
		return 0;
	case WM_HSCROLL:
		WindowHScroll(wParam, lParam);
		return 0;
	case WM_VSCROLL:
		WindowVScroll(wParam, lParam);
		return 0;
	case WM_MOUSEMOVE:
		WindowMouseMove(wParam, lParam);
		return 0;
	case WM_LBUTTONDOWN:
		WindowLButtonDown(wParam, lParam);
		return 0;
	case WM_LBUTTONUP:
		WindowLButtonUp(wParam, lParam);
		return 0;
	case WM_RBUTTONDOWN:
		WindowRButtonDown(wParam, lParam);
		return 0;
	case WM_RBUTTONUP:
		WindowRButtonUp(wParam, lParam);
		return 0;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	int i, cx;
	HICON Icon, hLogo, hLogo48;
	TOOLINFO ti;
	MIDIOUTCAPS moc;
	LOGBRUSH lb;
	char Buttons[48][256];
	char Tips[53][256];
	char sControllers[130][256];
	char path[MAX_PATH];
	SIZE size;
	DWORD dwStatus, dwOldStatus;

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	/* Initializes path to executable's directory */
	GetModuleFileName(NULL, &path[0], MAX_PATH);
	for (i=strlen(&path[0])-1; i >= 0; i--) if (path[i]=='\\') break; path[i]=0;
	SetCurrentDirectory(&path[0]);

	switch (GetUserDefaultLangID())
	{
	default:
		LoadText("strings.txt", (LPSTR) &Strings, STRINGS, 1024);
		LoadText("buttons.txt", (LPSTR) &Buttons, 48, 256);
		LoadText("tips.txt", (LPSTR) &Tips, 53, 256);
		LoadText("controllers.txt", (LPSTR) &sControllers, 130, 256);
		LoadText("instruments.txt", (LPSTR) &Instruments, 1408, 256);
		break;
	}

	hLogo = (HICON) LoadImage(NULL, "logo.ico", IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
	hLogo48 = (HICON) LoadImage(NULL, "logo48.ico", IMAGE_ICON, 48, 48, LR_LOADFROMFILE);

	splash = CreateWindow("STATIC", Strings[67], SS_CENTER | SS_CENTERIMAGE | WS_POPUP, GetSystemMetrics(SM_CXSCREEN) / 2 - 200, GetSystemMetrics(SM_CYSCREEN) / 2 - 40, 400, 80, NULL, NULL, hInstance, NULL);
	CreateWindow("STATIC", Strings[68], SS_CENTER | WS_CHILD | WS_VISIBLE, 0, 50, 400, 20, splash, (HMENU) 1, hInstance, NULL);
	CreateWindow("STATIC", NULL, SS_ETCHEDFRAME | WS_CHILD | WS_VISIBLE, -1, -1, 402, 82, splash, NULL, hInstance, NULL);
	SendMessage(splash, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);
	SendMessage(GetDlgItem(splash, 1), WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);
	SetClassLongPtr(splash, GCLP_HICON, (LONG_PTR) hLogo48);
	ShowWindow(splash, SW_SHOW);
	UpdateWindow(splash);

	iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	iccex.dwICC = ICC_UPDOWN_CLASS | ICC_WIN95_CLASSES;
	InitCommonControlsEx(&iccex);

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_CLASSDC;
	wc.hInstance = hInstance;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hIcon = hLogo48;
	wc.hIconSm = hLogo;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) (COLOR_3DFACE + 1);
	wc.lpszMenuName = NULL;
	wc.lpfnWndProc = &WindowProc;
	wc.lpszClassName = "WINDOW";
	RegisterClassEx(&wc);
	wc.lpfnWndProc = &TempoProc;
	wc.lpszClassName = "TEMPOWINDOW";
	RegisterClassEx(&wc);
	wc.lpfnWndProc = &ProgramProc;
	wc.lpszClassName = "PROGRAMWINDOW";
	RegisterClassEx(&wc);
	wc.lpfnWndProc = &FilterProc;
	wc.lpszClassName = "FILTERWINDOW";
	RegisterClassEx(&wc);

	window = CreateWindow("WINDOW", Strings[38], WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
	tooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_CLIPSIBLINGS | WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, window, NULL, hInstance, NULL);
	CreateWindow("COMBOBOX", NULL, WS_CLIPSIBLINGS | WS_VSCROLL | WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,   0, 56, 128, 200, window, (HMENU) ID_TRACK     , hInstance, NULL);
	CreateWindow("COMBOBOX", NULL, WS_CLIPSIBLINGS | WS_VSCROLL | WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 132, 56, 180, 200, window, (HMENU) ID_DEVICE    , hInstance, NULL);
	CreateWindow("COMBOBOX", NULL, WS_CLIPSIBLINGS | WS_VSCROLL | WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 316, 56,  64, 200, window, (HMENU) ID_ZOOM      , hInstance, NULL);
	CreateWindow("COMBOBOX", NULL, WS_CLIPSIBLINGS | WS_VSCROLL | WS_CHILD | WS_VISIBLE | CBS_DROPDOWN    , 384, 56,  96, 200, window, (HMENU) ID_BEAT      , hInstance, NULL);
	CreateWindow("COMBOBOX", NULL, WS_CLIPSIBLINGS | WS_VSCROLL | WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 484, 56, 128, 200, window, (HMENU) ID_CONTROLLER, hInstance, NULL);
	CreateWindow("SCROLLBAR", NULL, WS_CLIPSIBLINGS | WS_VISIBLE | WS_CHILD | SBS_HORZ, 1, 1, 1, 1, window, (HMENU) ID_POSITION, hInstance, NULL);
	CreateWindow("SCROLLBAR", NULL, WS_CLIPSIBLINGS | WS_VISIBLE | WS_CHILD | SBS_VERT, 1, 1, 1, 1, window, (HMENU) ID_NOTE    , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE,  0 * 24 +  0,  0, 24, 24, window, (HMENU) ID_FILENEW       , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE,  1 * 24 +  0,  0, 24, 24, window, (HMENU) ID_FILEOPEN      , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE,  2 * 24 +  0,  0, 24, 24, window, (HMENU) ID_FILESAVE      , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE,  3 * 24 +  0,  0, 24, 24, window, (HMENU) ID_FILESAVEAS    , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE,  4 * 24 +  0,  0, 24, 24, window, (HMENU) ID_FILTER        , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE,  5 * 24 +  0,  0, 24, 24, window, (HMENU) ID_DECODE        , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE,  6 * 24 +  4,  0, 24, 24, window, (HMENU) ID_PLAY          , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE,  7 * 24 +  4,  0, 24, 24, window, (HMENU) ID_HOME          , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE,  8 * 24 +  4,  0, 24, 24, window, (HMENU) ID_PAUSE         , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE,  9 * 24 +  4,  0, 24, 24, window, (HMENU) ID_END           , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 10 * 24 +  4,  0, 24, 24, window, (HMENU) ID_STOP          , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 11 * 24 +  8,  0, 24, 24, window, (HMENU) ID_MUTE          , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 12 * 24 +  8,  0, 24, 24, window, (HMENU) ID_SOLO          , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 13 * 24 +  8,  0, 24, 24, window, (HMENU) ID_LOOP          , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 14 * 24 + 12,  0, 24, 24, window, (HMENU) ID_INSERTTRACK   , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 15 * 24 + 12,  0, 24, 24, window, (HMENU) ID_REMOVETRACK   , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 16 * 24 + 16,  0, 24, 24, window, (HMENU) ID_COPYRIGHT     , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 17 * 24 + 16,  0, 24, 24, window, (HMENU) ID_TRACKNAME     , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 18 * 24 + 16,  0, 24, 24, window, (HMENU) ID_TRACKNUMBER   , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 19 * 24 + 16,  0, 24, 24, window, (HMENU) ID_INSTRUMENTNAME, hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 20 * 24 + 20,  0, 24, 24, window, (HMENU) ID_HELP          , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 21 * 24 + 20,  0, 24, 24, window, (HMENU) ID_API           , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 22 * 24 + 20,  0, 24, 24, window, (HMENU) ID_ABOUT         , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE,  0 * 24 +  0, 28, 24, 24, window, (HMENU) ID_CUT           , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE,  1 * 24 +  0, 28, 24, 24, window, (HMENU) ID_COPY          , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE,  2 * 24 +  0, 28, 24, 24, window, (HMENU) ID_PASTE         , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE,  3 * 24 +  0, 28, 24, 24, window, (HMENU) ID_DELETE        , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE,  4 * 24 +  0, 28, 24, 24, window, (HMENU) ID_SELECTALL     , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE,  5 * 24 +  4, 28, 24, 24, window, (HMENU) ID_TRANSPOSEUP   , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE,  6 * 24 +  4, 28, 24, 24, window, (HMENU) ID_TRANSPOSEDOWN , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE,  7 * 24 +  4, 28, 24, 24, window, (HMENU) ID_OCTAVEUP      , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE,  8 * 24 +  4, 28, 24, 24, window, (HMENU) ID_OCTAVEDOWN    , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE,  9 * 24 +  8, 28, 24, 24, window, (HMENU) ID_SELECT        , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 10 * 24 +  8, 28, 24, 24, window, (HMENU) ID_0             , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 11 * 24 +  8, 28, 24, 24, window, (HMENU) ID_1             , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 12 * 24 +  8, 28, 24, 24, window, (HMENU) ID_2             , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 13 * 24 +  8, 28, 24, 24, window, (HMENU) ID_4             , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 14 * 24 +  8, 28, 24, 24, window, (HMENU) ID_8             , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 15 * 24 +  8, 28, 24, 24, window, (HMENU) ID_16            , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 16 * 24 +  8, 28, 24, 24, window, (HMENU) ID_32            , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 17 * 24 +  8, 28, 24, 24, window, (HMENU) ID_64            , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 18 * 24 + 12, 28, 24, 24, window, (HMENU) ID_PROGRAM       , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 19 * 24 + 12, 28, 24, 24, window, (HMENU) ID_TEMPO         , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 20 * 24 + 12, 28, 24, 24, window, (HMENU) ID_TEXT          , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 21 * 24 + 12, 28, 24, 24, window, (HMENU) ID_LYRIC         , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 22 * 24 + 12, 28, 24, 24, window, (HMENU) ID_MARKER        , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 23 * 24 + 12, 28, 24, 24, window, (HMENU) ID_CUEPOINT      , hInstance, NULL);
	CreateWindow("BUTTON", NULL, WS_CLIPSIBLINGS | BS_ICON | WS_CHILD | WS_VISIBLE, 24 * 24 + 12, 28, 24, 24, window, (HMENU) ID_UNKNOWN       , hInstance, NULL);

	hdc = GetDC(window);
	SetBkMode(hdc, TRANSPARENT);
	SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));

	imagelist = ImageList_Create(16, 16, ILC_COLORDDB | ILC_MASK, 45, 45);
	lb.lbStyle = BS_SOLID;
	lb.lbColor = 0x0000FF;
	red = CreateBrushIndirect(&lb);
	lb.lbColor = 0xFF0000;
	blue = CreateBrushIndirect(&lb);
	lb.lbColor = 0x000000;
	black = CreateBrushIndirect(&lb);
	lb.lbColor = 0xFFFFFF;
	white = CreateBrushIndirect(&lb);
	lb.lbStyle = BS_HOLLOW;
	hollow = CreateBrushIndirect(&lb);
	dottedp = CreatePen(PS_DOT, 1, 0x000000);
	redp = CreatePen(PS_SOLID, 1, 0x0000FF);
	greenp = CreatePen(PS_SOLID, 1, 0x00FF00);
	nullp = CreatePen(PS_NULL, 1, 0x0000FF);
	blackp = CreatePen(PS_SOLID, 1, 0x000000);
	whitep = CreatePen(PS_SOLID, 1, 0xFFFFFF);
	gray1 = CreatePen(PS_SOLID, 1, 0x606060);
	gray2 = CreatePen(PS_SOLID, 1, 0x808080);
	gray3 = CreatePen(PS_SOLID, 1, 0xA0A0A0);
	gray4 = CreatePen(PS_SOLID, 1, 0xC0C0C0);
	gray5 = CreatePen(PS_SOLID, 1, 0xE0E0E0);
	hfont = CreateFont(14, (int) NULL, (int) NULL, (int) NULL, FW_BOLD, (DWORD) NULL, (DWORD) NULL, (DWORD) NULL, (DWORD) NULL, (DWORD) NULL, (DWORD) NULL, (DWORD) NULL, (DWORD) NULL, "Arial");
	Cursors[ARROW  ] = LoadCursor(NULL, IDC_ARROW  );
	Cursors[HAND   ] = LoadCursor(NULL, IDC_HAND   );
	Cursors[UPARROW] = LoadCursor(NULL, IDC_UPARROW);
	Cursors[SIZEALL] = LoadCursor(NULL, IDC_SIZEALL);
	Cursors[SIZEWE ] = LoadCursor(NULL, IDC_SIZEWE );

	for (i=ID_FILENEW; i<=ID_CONTROLLER; i++)
	{
		if (i<=ID_DECODE)
		{
			Icon = (HICON) LoadImage(NULL, (char*) &Buttons[i], IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
			ImageList_AddIcon(imagelist, Icon);
			SendMessage(GetDlgItem(window, i), BM_SETIMAGE, IMAGE_ICON, (LPARAM) Icon);
		}

		ti.cbSize = sizeof(TOOLINFO);
		ti.uFlags = TTF_SUBCLASS;
		ti.hwnd = GetDlgItem(window, i);
		ti.uId = i;
		ti.hinst = NULL;
		ti.rect.top = 0;
		ti.rect.bottom = 24;
		ti.rect.left = 0;
		ti.rect.right = 200;
		ti.lpszText = (char*) &Tips[i];
		ti.lParam = 0;
		SendMessage(tooltip, TTM_ADDTOOL, 0, (LPARAM) &ti);
	}

	for (i=0; i<200; i++)
	{
		SendMessage(GetDlgItem(window, ID_ZOOM), CB_ADDSTRING, 0, (LPARAM) strcat(itoa((i+1) * 5, lpstr1, 10), "%"));
		SendMessage(GetDlgItem(window, ID_BEAT), CB_ADDSTRING, 0, (LPARAM) strcat(itoa((i+1) * 16, lpstr1, 10), Strings[43]));
	}

	Saved = TRUE;
	Mode = ID_SELECT;
	Meta = ID_PROGRAM;
	Beat = 16;
	Zoom = 100;
	TickWidth = 8.0l;
	CursorSet = -1;
	nClipboard = 0;
	ClipboardBeat = 16;
	Clipboard = NULL;
	FirstFileOpen = TRUE;
	Device = -1;
	dwOldStatus = 0;

	EnumChildWindows(window, EnumChildProc, 0);

	cx = 0;
	for (i=0; i<130; i++)
	{
		SendMessage(GetDlgItem(window, ID_CONTROLLER), CB_ADDSTRING, 0, (LPARAM) &sControllers[i]);
		GetTextExtentPoint32(GetDC(GetDlgItem(window, ID_CONTROLLER)), (char*) &sControllers[i], strlen((char*) &sControllers[i]), &size);
		if (size.cx > cx ) cx = size.cx;
	}
	SendMessage(GetDlgItem(window, ID_CONTROLLER), CB_SETDROPPEDWIDTH, cx, 0);

	cx = 0;
	for (i=0; i<=(int)midiOutGetNumDevs(); i++)
	{
		midiOutGetDevCaps(i-1, &moc, sizeof(MIDIOUTCAPS));
		SendMessage(GetDlgItem(window, ID_DEVICE), CB_ADDSTRING, 0, (LPARAM) &moc.szPname);
		GetTextExtentPoint32(GetDC(GetDlgItem(window, ID_DEVICE)), (char*) &moc.szPname, strlen((char*) &moc.szPname), &size);
		if (size.cx > cx ) cx = size.cx;
	}
	SendMessage(GetDlgItem(window, ID_DEVICE), CB_SETDROPPEDWIDTH, cx, 0);

	if (FileNew())
	{
		SetWindowPos(window, NULL, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_NOMOVE | SWP_NOZORDER);
		AdjustWindow();

		ShowWindow(splash, SW_HIDE);

		ShowWindow(window, SW_SHOWMAXIMIZED);

		while (msg.message!=WM_QUIT)
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				DispatchMessage(&msg);
			else
			{
				dwStatus = MidiGet(Sequence, MIDI_STATUS);
				if ((dwStatus==MIDI_PLAY) || (dwOldStatus!=dwStatus))
					Callback();
				dwOldStatus = dwStatus;
				MsgWaitForMultipleObjects(0, NULL, TRUE, INFINITE, QS_ALLEVENTS);
			}

	}

	MidiCleanEvents(nClipboard, Clipboard);

	ImageList_Destroy(imagelist);
	DestroyIcon(wc.hIcon);
	DestroyCursor(wc.hCursor);
	DeleteObject(red);
	DeleteObject(blue);
	DeleteObject(black);
	DeleteObject(white);
	DeleteObject(hollow);
	DeleteObject(redp);
	DeleteObject(greenp);
	DeleteObject(dottedp);
	DeleteObject(nullp);
	DeleteObject(blackp);
	DeleteObject(whitep);
	DeleteObject(gray1);
	DeleteObject(gray2);
	DeleteObject(gray3);
	DeleteObject(gray4);
	DeleteObject(gray5);
	DeleteDC(buffer);
	DeleteObject(bitmap);
	DeleteObject(hfont);
	for (i=0; i<5; i++) DestroyCursor(Cursors[i]);

	return msg.wParam;
}

