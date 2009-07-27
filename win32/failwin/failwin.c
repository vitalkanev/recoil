/*
 * failwin.c - Windows API port of FAIL
 *
 * Copyright (C) 2009  Piotr Fusik and Adrian Matoga
 *
 * This file is part of FAIL (First Atari Image Library),
 * see http://fail.sourceforge.net
 *
 * FAIL is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * FAIL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FAIL; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <windows.h>
#include <string.h>

#include "fail.h"
#include "pngsave.h"
#include "failwin.h"

#define APP_TITLE        "FAILWin"
#define WND_CLASS_NAME   "FAILWin"

static HWND hWnd;
static byte atari_palette[FAIL_PALETTE_MAX + 1];
static BOOL use_atari_palette = FALSE;
static BOOL invert = FALSE;

static char current_filename[MAX_PATH] = "";
static byte image[FAIL_IMAGE_MAX];
static int image_len;
static int width = 0;
static int height;
static int colors;
static byte pixels[FAIL_PIXELS_MAX];
static byte palette[FAIL_PALETTE_MAX];

static void ShowError(const char *message)
{
	MessageBox(hWnd, message, APP_TITLE, MB_OK | MB_ICONERROR);
}

static BOOL LoadFile(const char *filename, byte *buffer, int *len)
{
	HANDLE fh;
	BOOL ok;
	fh = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (fh == INVALID_HANDLE_VALUE)
		return FALSE;
	ok = ReadFile(fh, buffer, *len, (LPDWORD) len, NULL);
	CloseHandle(fh);
	return ok;
}

static BOOL DecodeImage(const char *filename)
{
	return FAIL_DecodeImage(filename, image, image_len,
		use_atari_palette ? atari_palette : NULL,
		&width, &height, &colors, pixels, palette);
}

static void OpenImage(void)
{
	image_len = sizeof(image);
	if (!LoadFile(current_filename, image, &image_len)) {
		ShowError("Cannot open file");
		return;
	}
	if (!DecodeImage(current_filename)) {
		width = 0;
		ShowError("Decoding error");
		return;
	}
	InvalidateRect(hWnd, NULL, TRUE);
}

static void SelectAndOpenImage(void)
{
	static OPENFILENAME ofn = {
		sizeof(OPENFILENAME),
		NULL,
		0,
		"All supported\0"
		"*.ap3;*.apc;*.cci;*.cin;*.cpr;*.gr8;*.gr9;*.hip;*.hr;*.ilc;*.inp;*.int;*.mic;*.pic;*.plm;*.rip;*.tip\0"
		"\0",
		NULL,
		0,
		1,
		current_filename,
		MAX_PATH,
		NULL,
		0,
		NULL,
		"Select 8-bit Atari image",
		OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
		0,
		0,
		NULL,
		0,
		NULL,
		NULL
	};
	ofn.hwndOwner = hWnd;
	if (GetOpenFileName(&ofn))
		OpenImage();
}

static int GetPathLength(const char *filename)
{
	int i;
	int len = 0;
	for (i = 0; filename[i] != '\0'; i++)
		if (filename[i] == '\\' || filename[i] == '/')
			len = i + 1;
	return len;
}

static BOOL GetSiblingFile(char *filename, int dir)
{
	int len;
	char mask[MAX_PATH];
	char best[MAX_PATH];
	HANDLE fh;
	WIN32_FIND_DATA wfd;
	len = GetPathLength(filename);
	if (len > MAX_PATH - 2)
		return FALSE;
	memcpy(mask, filename, len);
	mask[len] = '*';
	mask[len + 1] = '\0';
	best[0] = '\0';
	fh = FindFirstFile(mask, &wfd);
	if (fh == INVALID_HANDLE_VALUE)
		return FALSE;
	do {
		if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0
			&& wfd.nFileSizeHigh == 0
			&& wfd.nFileSizeLow <= FAIL_IMAGE_MAX
			&& FAIL_IsOurFile(wfd.cFileName)
			&& _stricmp(wfd.cFileName, filename + len) * dir > 0) {
			if (best[0] == '\0'
			 || _stricmp(wfd.cFileName, best) * dir < 0) {
				strcpy(best, wfd.cFileName);
			}
		}
	} while (FindNextFile(fh, &wfd));
	FindClose(fh);
	if (best[0] == '\0')
		return FALSE;
	if (len + strlen(best) + 1 >= MAX_PATH)
		return FALSE;
	strcpy(filename + len, best);
	return TRUE;
}

static void OpenSiblingImage(int dir)
{
	char filename[MAX_PATH];
	strcpy(filename, current_filename);
	while (GetSiblingFile(filename, dir)) {
		image_len = sizeof(image);
		if (LoadFile(filename, image, &image_len) && DecodeImage(filename)) {
			strcpy(current_filename, filename);
			InvalidateRect(hWnd, NULL, TRUE);
			return;
		}
	}
}

static void SelectAndSaveImage(void)
{
	static char png_filename[MAX_PATH] = "";
	static OPENFILENAME ofn = {
		sizeof(OPENFILENAME),
		NULL,
		0,
		"PNG images (*.png)\0*.png\0\0",
		NULL,
		0,
		1,
		png_filename,
		MAX_PATH,
		NULL,
		0,
		NULL,
		"Select output file",
		OFN_ENABLESIZING | OFN_EXPLORER | OFN_OVERWRITEPROMPT,
		0,
		0,
		"png",
		0,
		NULL,
		NULL
	};
	ofn.hwndOwner = hWnd;
	if (!GetSaveFileName(&ofn))
		return;
	if (!PNG_Save(png_filename, width, height, colors, pixels, palette))
		ShowError("Error writing file");
}

static void SelectAndOpenPalette(void)
{
	static char act_filename[MAX_PATH] = "";
	static OPENFILENAME ofn = {
		sizeof(OPENFILENAME),
		NULL,
		0,
		"Palette files (*.act)\0"
		"*.act\0"
		"\0",
		NULL,
		0,
		1,
		act_filename,
		MAX_PATH,
		NULL,
		0,
		NULL,
		"Select 8-bit Atari palette",
		OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
		0,
		0,
		NULL,
		0,
		NULL,
		NULL
	};
	ofn.hwndOwner = hWnd;
	if (GetOpenFileName(&ofn)) {
		int palette_len = sizeof(atari_palette);
		use_atari_palette = FALSE;
		if (!LoadFile(act_filename, atari_palette, &palette_len)) {
			ShowError("Cannot open file");
			return;
		}
		if (palette_len != FAIL_PALETTE_MAX) {
			ShowError("Invalid file length - must be 768 bytes");
			return;
		}
		use_atari_palette = TRUE;
		OpenImage();
	}
}

static void SwapRedAndBlue(void)
{
	byte *p;
	for (p = pixels + width * height * 3; (p -= 3) >= pixels; ) {
		byte t = p[0];
		p[0] = p[2];
		p[2] = t;
	}
}

static void CopyColor(RGBQUAD *dest, int i)
{
	dest->rgbRed = palette[3 * i];
	dest->rgbGreen = palette[3 * i + 1];
	dest->rgbBlue = palette[3 * i + 2];
}

static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int idc;
	PCOPYDATASTRUCT pcds;
	switch (msg) {
	case WM_PAINT:
		if (width > 0) {
			PAINTSTRUCT ps;
			HDC hdc;
			RECT rect;
			struct {
				BITMAPINFOHEADER bmiHeader;
				RGBQUAD bmiColors[256];
			} bmi;
			int i;
			bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmi.bmiHeader.biWidth = width;
			bmi.bmiHeader.biHeight = -height;
			bmi.bmiHeader.biPlanes = 1;
			bmi.bmiHeader.biBitCount = colors <= 256 ? 8 : 24;
			bmi.bmiHeader.biCompression = BI_RGB;
			bmi.bmiHeader.biSizeImage = 0;
			bmi.bmiHeader.biXPelsPerMeter = 1000;
			bmi.bmiHeader.biYPelsPerMeter = 1000;
			bmi.bmiHeader.biClrUsed = 0;
			bmi.bmiHeader.biClrImportant = 0;
			if (colors <= 256) {
				if (colors == 2 && invert) {
					CopyColor(bmi.bmiColors, 1);
					CopyColor(bmi.bmiColors + 1, 0);
				}
				else
				{
					for (i = 0; i < colors; i++)
						CopyColor(bmi.bmiColors + i, i);
				}
			}
			else
				SwapRedAndBlue();
			hdc = BeginPaint(hWnd, &ps);
			GetClientRect(hWnd, &rect);
			StretchDIBits(hdc, 0, 0, rect.right, rect.bottom, 0, 0, width, height,
				pixels, (CONST BITMAPINFO *) &bmi, DIB_RGB_COLORS, SRCCOPY);
			EndPaint(hWnd, &ps);
			if (colors > 256)
				SwapRedAndBlue();
		}
		break;
	case WM_LBUTTONDOWN:
		OpenSiblingImage(1);
		break;
	case WM_RBUTTONDOWN:
		OpenSiblingImage(-1);
		break;
	case WM_COMMAND:
		idc = LOWORD(wParam);
		switch (idc) {
		case IDM_OPEN:
			SelectAndOpenImage();
			break;
		case IDM_PREVFILE:
			OpenSiblingImage(-1);
			break;
		case IDM_NEXTFILE:
			OpenSiblingImage(1);
			break;
		case IDM_SAVEAS:
			SelectAndSaveImage();
			break;
		case IDM_LOADPALETTE:
			SelectAndOpenPalette();
			break;
		case IDM_EXIT:
			PostQuitMessage(0);
			break;
		case IDM_INVERT:
			invert = !invert;
			InvalidateRect(hWnd, NULL, TRUE);
			break;
		case IDM_ABOUT:
			MessageBox(hWnd,
				FAIL_CREDITS
				"\n"
				FAIL_COPYRIGHT,
				APP_TITLE " " FAIL_VERSION,
				MB_OK | MB_ICONINFORMATION);
			break;
		default:
			break;
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_COPYDATA:
		pcds = (PCOPYDATASTRUCT) lParam;
		if (pcds->dwData == 'O' && pcds->cbData <= sizeof(current_filename)) {
			memcpy(current_filename, pcds->lpData, pcds->cbData);
			OpenImage();
		}
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	char *pb;
	char *pe;
	WNDCLASS wc;
	HACCEL hAccel;
	MSG msg;

	for (pb = lpCmdLine; *pb == ' ' || *pb == '\t'; pb++);
	for (pe = pb; *pe != '\0'; pe++);
	while (--pe > pb && (*pe == ' ' || *pe == '\t'));
	/* Now pb and pe point at respectively the first and last non-blank
	   character in lpCmdLine. If pb > pe then the command line is blank. */
	if (*pb == '"' && *pe == '"')
		pb++;
	else
		pe++;
	*pe = '\0';
	/* Now pb contains the filename, if any, specified on the command line. */

	hWnd = FindWindow(WND_CLASS_NAME, NULL);
	if (hWnd != NULL) {
		/* an instance of FAILWin is already running */
		if (*pb != '\0') {
			/* pass the filename */
			COPYDATASTRUCT cds = { 'O', (DWORD) (pe + 1 - pb), pb };
			SendMessage(hWnd, WM_COPYDATA, (WPARAM) NULL, (LPARAM) &cds);
		}
		else {
			/* bring the open dialog to top */
			HWND hChild = GetLastActivePopup(hWnd);
			if (hChild != hWnd)
				SetForegroundWindow(hChild);
		}
		return 0;
	}

	wc.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = NULL; // TODO LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU);
	wc.lpszClassName = WND_CLASS_NAME;
	RegisterClass(&wc);

	hWnd = CreateWindow(WND_CLASS_NAME,
		APP_TITLE,
		WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATORS));

	if (*pb != '\0') {
		memcpy(current_filename, pb, pe + 1 - pb);
		OpenImage();
	}
	else
		SelectAndOpenImage();

	while (GetMessage(&msg, NULL, 0, 0)) {
		if (!TranslateAccelerator(hWnd, hAccel, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return 0;
}
