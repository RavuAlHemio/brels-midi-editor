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

/* Search formats */
#define MIDI_TICKS                101
#define MIDI_TIME                 102
#define MIDI_EVENT                103
#define MIDI_INDEX                104
#define MIDI_FILTER               105

/* Playback stati */
#define MIDI_BUSY                 201
#define MIDI_STOP                 202
#define MIDI_PAUSE                203
#define MIDI_PLAY                 204

/* General Properties */
#define BEAT_SIZE                 300
#define SOLO_TRACK                301
#define TICK_INTERVAL             302
#define SKIP_INTERVAL             303
#define BYTE_COUNT                305
#define TICK_COUNT                306
#define TIME_COUNT                307
#define EVENT_COUNT               308
#define TRACK_COUNT               309
#define CURRENT_TIME              310
#define CURRENT_TICKS             311
#define CURRENT_TEMPO             312
#define CALLBACK_HWND             313
#define CALLBACK_MESSAGE          314
#define CALLBACK_PROCESSED        315
#define MIDI_DEVICE               316
#define MIDI_HANDLE               317
#define MIDI_HEADER               318
#define MIDI_LOOP                 319
#define MIDI_PRECISION            320
#define MIDI_STATUS               321
#define MIDI_DONE                 322


/* Track Properties */
#define CURRENT_EVENT             300
#define TRACK_DATA                301
#define TRACK_HEADER              302
#define TRACK_MUTE                303
#define TRACK_INDEX               304


/* Event properties */
#define EVENT_INDEX               400
#define EVENT_TIME                401
#define EVENT_TICKS               402
#define EVENT_EVENT               403
#define EVENT_DATA                404
#define EVENT_DATASIZE            405
#define EVENT_TAG                 406


/* Filters */
#define FILTER_SEQUENCENUMBER      0x00000001ul
#define FILTER_TEXT                0x00000002ul
#define FILTER_COPYRIGHT           0x00000004ul
#define FILTER_TRACKNAME           0x00000008ul
#define FILTER_INSTRUMENTNAME      0x00000010ul
#define FILTER_LYRIC               0x00000020ul
#define FILTER_MARKER              0x00000040ul
#define FILTER_CUEPOINT            0x00000080ul
#define FILTER_CHANNELPREFIX       0x00000100ul
#define FILTER_ENDOFTRACK          0x00000200ul
#define FILTER_TEMPO               0x00000400ul
#define FILTER_SMPTEOFFSET         0x00000800ul
#define FILTER_TIMESIGNATURE       0x00001000ul
#define FILTER_KEYSIGNATURE        0x00002000ul
#define FILTER_SEQUENCERSPECIFIC   0x00004000ul
#define FILTER_SYSTEMEXCLUSIVE     0x00008000ul
#define FILTER_NOTEOFF             0x00010000ul
#define FILTER_NOTEON              0x00020000ul
#define FILTER_KEYAFTERTOUCH       0x00040000ul
#define FILTER_CONTROLLER          0x00080000ul
#define FILTER_PROGRAM             0x00100000ul
#define FILTER_CHANNELPRESSURE     0x00200000ul
#define FILTER_PITCHWHEEL          0x00400000ul
#define FILTER_MTCQUARTERFRAME     0x00800000ul
#define FILTER_SONGPOSITIONPOINTER 0x01000000ul
#define FILTER_SONGSELECT          0x02000000ul
#define FILTER_TUNEREQUEST         0x04000000ul
/*
#define FILTER_MIDICLOCK           0x08000000ul
#define FILTER_MIDISTART           0x10000000ul
#define FILTER_MIDICONTINUE        0x20000000ul
#define FILTER_MIDISTOP            0x40000000ul
*/
#define FILTER_ACTIVESENSE         0x08000000ul
#define FILTER_NOTES               0x10000000ul
#define FILTER_OTHER               0x20000000ul
#define FILTER_ALL                 0x40000000ul


/* General purpose constants */
#define DEFAULT_DEVICE             (DWORD) -1
#define NO_DEVICE                  0xFFFFFFFE
#define NO_SOLO                    0xFFFFFFFF
#define BRELS_ERROR                0xFFFFFFFF

/* 64-bit unsigned integer used in the library */
typedef ULONGLONG QWORD, *LPQWORD;


/* MIDI header as found in a MIDI file */
typedef struct
{
	DWORD dwMagic;
	DWORD dwSize;
	WORD  wFormat;
	WORD  wTracks;
	WORD  wBeatSize;
}  __attribute__ ((packed, aligned(1)))
BRELS_MIDI_HEADER, *LPBRELS_MIDI_HEADER;


/* MIDI track header as found in a MIDI file */
typedef struct
{
	DWORD dwMagic;
	DWORD dwLength;
} BRELS_TRACK_HEADER, *LPBRELS_TRACK_HEADER;


/* BRELS style MIDI Event */
typedef struct
{
	WORD wTag;
	DWORD dwTicks;
	BYTE Event;
	BYTE DataSize;
	union
	{
		BYTE a[4];
		LPBYTE p;
	} Data;
} __attribute__ ((packed, aligned(1)))
BRELS_MIDI_EVENT, *LPBRELS_MIDI_EVENT;


/* Filtered event for simplification */
typedef struct
{
	QWORD qwFilter;
	QWORD qwStart;
	QWORD qwEnd;
} BRELS_FILTERED_EVENT, *LPBRELS_FILTERED_EVENT;

typedef struct
{
	WORD wTrack;
	DWORD dwEvent;
} __attribute__ ((packed, aligned(1)))
BRELS_TEMPO, *LPBRELS_TEMPO;


BOOL  WINAPI MidiOpen(LPSTR lpstrFile, DWORD dwDevice, LPHANDLE lphSequence);
BOOL  WINAPI MidiClose(HANDLE hSequence);
BOOL  WINAPI MidiCreate(DWORD dwDevice, WORD wBeatSize, LPHANDLE lphSequence);
BOOL  WINAPI MidiEncode(HANDLE hSequence, LPSTR lpstrFile, BOOL OverWrite);

BOOL  WINAPI MidiPlay(HANDLE hSequence);
BOOL  WINAPI MidiPause(HANDLE hSequence);
BOOL  WINAPI MidiStop(HANDLE hSequence);
BOOL  WINAPI MidiNext(HANDLE hSequence);
BOOL  WINAPI MidiSuspend(HANDLE hSequence, BOOL Synchronize);
BOOL  WINAPI MidiResume(HANDLE hSequence, BOOL Modified);
BOOL  WINAPI MidiSilence(HANDLE hSequence);
BOOL  WINAPI MidiReset(HANDLE hSequence);

BOOL  WINAPI MidiInsertTrack(HANDLE hSequence, WORD wTrack);
BOOL  WINAPI MidiRemoveTrack(HANDLE hSequence, WORD wTrack);
DWORD WINAPI MidiInsertTrackEvents(HANDLE hSequence, WORD wTrack, DWORD dwEvents, LPBRELS_MIDI_EVENT lpEvents);
DWORD WINAPI MidiRemoveTrackEvents(HANDLE hSequence, WORD wTrack, QWORD qwFirst, QWORD qwLast, DWORD dwFormat);
DWORD WINAPI MidiGetTrackEvents(HANDLE hSequence, WORD wTrack, QWORD qwFirst, QWORD qwLast, DWORD dwFormat, LPBRELS_MIDI_EVENT* lpEvents);
DWORD WINAPI MidiFilterTrackEvents(HANDLE hSequence, WORD wTrack, QWORD qwFirst, QWORD qwLast, DWORD dwFormat, QWORD qwFilter, LPBRELS_FILTERED_EVENT* lpFilteredEvents);
DWORD WINAPI MidiCleanEvents(DWORD dwEvents, LPBRELS_MIDI_EVENT lpEvents);
BOOL  WINAPI MidiFreeBuffer(LPVOID lpBuffer);

QWORD WINAPI MidiGet(HANDLE hSequence, DWORD dwWhat);
QWORD WINAPI MidiTrackGet(HANDLE hSequence, WORD wTrack, DWORD dwWhat);
QWORD WINAPI MidiEventGet(HANDLE hSequence, WORD wTrack, DWORD dwEvent, DWORD dwWhat);
QWORD WINAPI MidiSet(HANDLE hSequence, DWORD dwWhat, QWORD qwValue);
QWORD WINAPI MidiTrackSet(HANDLE hSequence, WORD wTrack, DWORD dwWhat, QWORD qwValue);
QWORD WINAPI MidiEventSet(HANDLE hSequence, WORD wTrack, DWORD dwEvent, DWORD dwWhat, QWORD qwValue);

QWORD WINAPI MidiQuery(HANDLE hSequence, WORD wTrack, DWORD dwWhat, DWORD dwFrom, QWORD qwValue);
WORD  WINAPI btlw(WORD big);   /* Big-endian word to little-endian (vice-versa) */
DWORD WINAPI btldw(DWORD big); /* Big-endian dword to little-endian (vice-versa) */
