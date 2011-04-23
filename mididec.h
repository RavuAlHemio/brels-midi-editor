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

int DecoderDialog(HWND hwndParent, HANDLE Sequence);
LRESULT CALLBACK DecoderDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
