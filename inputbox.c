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
#include "inputbox.h"

LRESULT CALLBACK InputBoxProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
		SendMessage(hwnd, WM_COMMAND, IDCANCEL, 0);
		return 0;
	case WM_DESTROY:
		UnregisterClass("INPUTBOXCLASS", (HINSTANCE) GetClassLongPtr(hwnd, GCLP_HMODULE));
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
	default:
		break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int InputBox(HWND hwndParent, char* InputTitle, char* InputText, char* lpInOutText)
{
	WNDCLASS wc;
	int Width, Height;
	HWND hwnd;
	MSG msg;
	int Return;

	if (!IsWindow(hwndParent)) return 0;
	/*if (IsBadCodePtr((FARPROC) lpInOutText)) return (int) NULL;*/

	ZeroMemory(&wc, sizeof(WNDCLASS));
	wc.style = CS_OWNDC | CS_DBLCLKS;
	wc.lpfnWndProc = InputBoxProc;
	wc.hInstance = (HINSTANCE) GetClassLongPtr(hwndParent, GCLP_HMODULE);
	wc.hIcon = (HICON) LoadIcon(NULL, IDI_QUESTION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) (COLOR_3DFACE + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "INPUTBOX";

	RegisterClass(&wc);

	Width = 300 + GetSystemMetrics(SM_CXFIXEDFRAME) * 2;
	Height = 100 + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFIXEDFRAME) * 2;

	hwnd = CreateWindow("INPUTBOX", InputTitle, WS_VISIBLE | WS_POPUP | WS_CAPTION | WS_SYSMENU, GetSystemMetrics(SM_CXSCREEN) / 2 - Width / 2, GetSystemMetrics(SM_CYSCREEN) / 2 - Height / 2, Width, Height, hwndParent, NULL, wc.hInstance, NULL);
	SendMessage(CreateWindow("STATIC", InputText, WS_VISIBLE | WS_CHILD, 4, 4, 292, 48, hwnd, NULL, wc.hInstance, NULL), WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);
	SendMessage(CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", lpInOutText, WS_TABSTOP | WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, 4, 52, 292, 20, hwnd, (HMENU) 10000, wc.hInstance, NULL), WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);
	SendMessage(CreateWindow("BUTTON", "OK", WS_TABSTOP | WS_VISIBLE | WS_CHILD , 164, 76, 64, 20, hwnd, (HMENU) IDOK, wc.hInstance, NULL), WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);
	SendMessage(CreateWindow("BUTTON", "Cancel", WS_TABSTOP | WS_VISIBLE | WS_CHILD, 232, 76, 64, 20, hwnd, (HMENU) IDCANCEL, wc.hInstance, NULL), WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) IDCANCEL);

	SetFocus(GetDlgItem(hwnd, 10000));
	SendMessage(GetDlgItem(hwnd, 10000), EM_SETSEL, 0, -1);

	EnableWindow(hwndParent, FALSE);

	while (IsWindowVisible(hwnd))
	if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	if (!IsDialogMessage(GetActiveWindow(), &msg))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	Return = (int) GetWindowLongPtr(hwnd, GWLP_USERDATA);

	if (Return == IDOK)
	{
		GetWindowText(
					GetDlgItem(hwnd, 10000),
					lpInOutText,
					(int)(SendMessage(GetDlgItem(hwnd, 10000), WM_GETTEXTLENGTH, 0, 0) + 1)
				 );
	}

	DestroyWindow(hwnd);

	return Return;
}
