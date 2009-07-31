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
#include <stdio.h>
#include <string.h>

#include "fail.h"
#include "pngsave.h"
#include "failwin.h"

#define ZOOM_STEP               10
#define ZOOM_MIN                100
#define WINDOW_WIDTH_MIN        100
#define WINDOW_HEIGHT_MIN       100
#define APP_TITLE               "FAILWin"
#define WND_CLASS_NAME          "FAILWin"

static HWND hWnd;
static HMENU hMenu;
static byte atari_palette[FAIL_PALETTE_MAX + 1];
static BOOL use_atari_palette = FALSE;
static BOOL fullscreen = FALSE;
static int zoom = 100;

static char current_filename[MAX_PATH] = "";
static byte image[FAIL_IMAGE_MAX];
static int image_len;
static int width = 0;
static int height;
static int colors;
static byte pixels[FAIL_PIXELS_MAX];
static byte palette[FAIL_PALETTE_MAX];
static int show_width;
static int show_height;
static int window_width;
static int window_height;

static struct {
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD bmiColors[256];
	byte pixels[FAIL_PIXELS_MAX];
} bitmap;
static byte *bitmap_pixels;

static void UpdateBitmap(void)
{
	bitmap.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmap.bmiHeader.biWidth = width;
	bitmap.bmiHeader.biHeight = height;
	bitmap.bmiHeader.biPlanes = 1;
	bitmap.bmiHeader.biBitCount = colors <= 256 ? 8 : 24;
	bitmap.bmiHeader.biCompression = BI_RGB;
	bitmap.bmiHeader.biXPelsPerMeter = 1000;
	bitmap.bmiHeader.biYPelsPerMeter = 1000;
	if (colors <= 256) {
		int i;
		int y;
		bitmap.bmiHeader.biSizeImage = sizeof(BITMAPINFOHEADER) + colors * sizeof(RGBQUAD) + width * height;
		bitmap.bmiHeader.biClrUsed = colors;
		bitmap.bmiHeader.biClrImportant = colors;
		for (i = 0; i < colors; i++) {
			bitmap.bmiColors[i].rgbRed = palette[3 * i];
			bitmap.bmiColors[i].rgbGreen = palette[3 * i + 1];
			bitmap.bmiColors[i].rgbBlue = palette[3 * i + 2];
		}
		bitmap_pixels = (byte *) (bitmap.bmiColors + colors);
		for (y = 0; y < height; y++)
			memcpy(bitmap_pixels + (height - 1 - y) * width, pixels + y * width, width);
	}
	else {
		int y;
		bitmap.bmiHeader.biSizeImage = sizeof(BITMAPINFOHEADER) + width * height * 3;
		bitmap.bmiHeader.biClrUsed = 0;
		bitmap.bmiHeader.biClrImportant = 0;
		bitmap_pixels = (byte *) bitmap.bmiColors;
		for (y = 0; y < height; y++) {
			const byte *p = pixels + y * width * 3;
			byte *q = bitmap_pixels + (height - 1 - y) * width * 3;
			int i;
			for (i = 0; i < width * 3; i += 3) {
				q[i] = p[i + 2];
				q[i + 1] = p[i + 1];
				q[i + 2] = p[i];
			}
		}
	}
}

static void ShowError(const char *message)
{
	MessageBox(hWnd, message, APP_TITLE, MB_OK | MB_ICONERROR);
}

static int Fit(int dest_width, int dest_height)
{
	if (width * dest_height < height * dest_width) {
		show_width = MulDiv(width, dest_height, height);
		show_height = dest_height;
		return MulDiv(100, dest_height, height);
	}
	else {
		show_width = dest_width;
		show_height = MulDiv(height, dest_width, width);
		return MulDiv(100, dest_width, width);
	}
}

static void CalculateWindowSize(void)
{
	window_width = show_width + GetSystemMetrics(SM_CXFIXEDFRAME) * 2;
	window_height = show_height + GetSystemMetrics(SM_CYFIXEDFRAME) * 2 + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYMENU);
	if (window_width < WINDOW_WIDTH_MIN)
		window_width = WINDOW_WIDTH_MIN;
	if (window_height < WINDOW_HEIGHT_MIN)
		window_height = WINDOW_HEIGHT_MIN;
}

static void ResizeWindow(void)
{
	RECT rect;
	if (!GetWindowRect(hWnd, &rect))
		return;
	MoveWindow(hWnd,
		(rect.left + rect.right - window_width) >> 1,
		(rect.top + rect.bottom - window_height) >> 1,
		window_width, window_height, TRUE);
}

static void Repaint(void)
{
	if (width > 0 && height > 0) {
		if (fullscreen)
			Fit(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
		else {
			int desktop_width = GetSystemMetrics(SM_CXFULLSCREEN);
			int desktop_height = GetSystemMetrics(SM_CYFULLSCREEN);
			RECT rect;
			show_width = MulDiv(width, zoom, 100);
			show_height = MulDiv(height, zoom, 100);
			CalculateWindowSize();
			if (window_width > desktop_width || window_height > desktop_height) {
				zoom = Fit(desktop_width, desktop_height);
				CalculateWindowSize();
			}
			ResizeWindow();
			GetClientRect(hWnd, &rect);
			if (rect.bottom < show_height) {
				window_height += show_height - rect.bottom;
				ResizeWindow();
			}
		}
	}
	InvalidateRect(hWnd, NULL, TRUE);
}

static void ToggleFullscreen(void)
{
	if (fullscreen) {
		ShowCursor(TRUE);
		SetWindowLong(hWnd, GWL_STYLE, WS_VISIBLE | WS_CAPTION | WS_SYSMENU);
		SetMenu(hWnd, hMenu);
		fullscreen = FALSE;
	}
	else {
		ShowCursor(FALSE);
		SetWindowLong(hWnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);
		SetMenu(hWnd, NULL);
		MoveWindow(hWnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), TRUE);
		fullscreen = TRUE;
	}
	Repaint();
}

static void ZoomIn(void)
{
	if (fullscreen)
		return;
	zoom += ZOOM_STEP;
	Repaint();
}

static void ZoomOut(void)
{
	if (fullscreen)
		return;
	zoom -= ZOOM_STEP;
	if (zoom < ZOOM_MIN)
		zoom = ZOOM_MIN;
	Repaint();
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

static int GetPathLength(const char *filename)
{
	int i;
	int len = 0;
	for (i = 0; filename[i] != '\0'; i++)
		if (filename[i] == '\\' || filename[i] == '/')
			len = i + 1;
	return len;
}

static void ShowImage(void)
{
	char title[MAX_PATH + 32];
	sprintf(title, "%s - " APP_TITLE, current_filename + GetPathLength(current_filename));
	SetWindowText(hWnd, title);
	UpdateBitmap();
	Repaint();
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
		SetWindowText(hWnd, APP_TITLE);
		ShowError("Decoding error");
		return;
	}
	ShowImage();
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
			ShowImage();
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

static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int idc;
	PCOPYDATASTRUCT pcds;
	switch (msg) {
	case WM_PAINT:
		if (width > 0 && height > 0) {
			PAINTSTRUCT ps;
			HDC hdc;
			RECT rect;
			int x;
			int y;
			GetClientRect(hWnd, &rect);
			x = rect.right > show_width ? (rect.right - show_width) >> 1 : 0;
			y = rect.bottom > show_height ? (rect.bottom - show_height) >> 1 : 0;
			hdc = BeginPaint(hWnd, &ps);
			StretchDIBits(hdc, x, y, show_width, show_height, 0, 0, width, height,
				bitmap_pixels, (CONST BITMAPINFO *) &bitmap, DIB_RGB_COLORS, SRCCOPY);
			EndPaint(hWnd, &ps);
		}
		break;
	case WM_LBUTTONDOWN:
		OpenSiblingImage(1);
		break;
	case WM_RBUTTONDOWN:
		OpenSiblingImage(-1);
		break;
	case WM_MOUSEWHEEL:
		if (GET_WHEEL_DELTA_WPARAM(wParam) > 0)
			ZoomIn();
		else
			ZoomOut();
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
		case IDM_COPY:
			if (width > 0 && height > 0) {
				if (OpenClipboard(hWnd)) {
					void *p = (void *) GlobalAlloc(GMEM_FIXED, bitmap.bmiHeader.biSizeImage);
					if (p != NULL) {
						memcpy(p, &bitmap, bitmap.bmiHeader.biSizeImage);
						EmptyClipboard();
						SetClipboardData(CF_DIB, GlobalHandle(p));
					}
					CloseClipboard();
				}
			}
			break;
		case IDM_FULLSCREEN:
			ToggleFullscreen();
			break;
		case IDM_ZOOMIN:
			ZoomIn();
			break;
		case IDM_ZOOMOUT:
			ZoomOut();
			break;
		case IDM_INVERT:
			if (colors == 2) {
				RGBQUAD tmp = bitmap.bmiColors[0];
				bitmap.bmiColors[0] = bitmap.bmiColors[1];
				bitmap.bmiColors[1] = tmp;
				Repaint();
			}
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
		WS_VISIBLE | WS_CAPTION | WS_SYSMENU,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		hInstance,
		NULL
	);
	hMenu = GetMenu(hWnd);

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
