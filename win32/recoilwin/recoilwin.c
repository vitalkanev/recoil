/*
 * recoilwin.c - Windows API port of RECOIL
 *
 * Copyright (C) 2009-2021  Piotr Fusik
 *
 * This file is part of RECOIL (Retro Computer Image Library),
 * see http://recoil.sourceforge.net
 *
 * RECOIL is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * RECOIL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RECOIL; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#define UNICODE 1
#define _UNICODE 1
#define PRIts "ls"

#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "recoil-win32.h"
#include "formats.h"
#include "pngsave.h"
#include "recoilwin.h"

#define ZOOM_STEP               10
#define ZOOM_MIN                100
#define WINDOW_WIDTH_MIN        200
#define WINDOW_HEIGHT_MIN       100
#define APP_TITLE               "RECOILWin"
#define WND_CLASS_NAME          _T("RECOILWin")

static HINSTANCE hInst;
static HWND hWnd;
static HMENU hMenu;
static HWND hStatus;

static RECOIL *recoil;
static _TCHAR image_filename[MAX_PATH] = _T("");
static bool image_loaded = false;

static bool skip_files = true;
static bool fullscreen = false;
static int zoom = 100;
static int show_width;
static int show_height;
static int window_width;
static int window_height;
static bool show_path = false;
static bool status_bar = true;

static struct {
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD bmiColors[256];
	BYTE pixels[RECOIL_MAX_PIXELS_LENGTH * 3];
} bitmap;
static BYTE *bitmap_pixels;

static void ShowError(const char *message)
{
	MessageBoxA(hWnd, message, APP_TITLE, MB_OK | MB_ICONERROR);
}

static void ShowAbout(void)
{
	MSGBOXPARAMSA mbp = {
		sizeof(MSGBOXPARAMSA),
		hWnd,
		hInst,
		"Retro Computer Image Library\n"
		"(C) " RECOIL_YEARS " Piotr Fusik and Adrian Matoga\n"
		"Formats research, testing by Mariusz Rozwadowski\n"
		"Icon by Pawel Szewczyk\n\n"
		RECOIL_COPYRIGHT,
		APP_TITLE " " RECOIL_VERSION,
		MB_OK | MB_USERICON,
		MAKEINTRESOURCEA(IDI_APP),
		0,
		NULL,
		LANG_NEUTRAL
	};
	MessageBoxIndirectA(&mbp);
}

static void SetMenuEnabled(int id, bool enable)
{
	EnableMenuItem(hMenu, id, MF_BYCOMMAND | (enable ? MF_ENABLED : MF_GRAYED));
}

static void SetMenuCheck(int id, bool check)
{
	CheckMenuItem(hMenu, id, MF_BYCOMMAND | (check ? MF_CHECKED : MF_UNCHECKED));
}

static int GetPathLength(LPCTSTR filename)
{
	int len = 0;
	for (int i = 0; filename[i] != '\0'; i++)
		if (filename[i] == '\\' || filename[i] == '/')
			len = i + 1;
	return len;
}

static void UpdateText(void)
{
	LPCTSTR filename = image_filename;
	if (filename[0] == '\0')
		return;
	if (!show_path)
		filename += GetPathLength(filename);
	_TCHAR buf[MAX_PATH + 64];
	_stprintf(buf, MAX_PATH + 64, _T("%" PRIts " - " APP_TITLE), filename);
	SetWindowText(hWnd, buf);
	int frames = RECOIL_GetFrames(recoil);
	if (image_loaded) {
		int colors = RECOIL_GetColors(recoil);
		_stprintf(buf, MAX_PATH + 64, _T("%s, %dx%d, %d color%s, %s%d%% zoom"),
			RECOIL_GetPlatform(recoil),
			RECOIL_GetOriginalWidth(recoil), RECOIL_GetOriginalHeight(recoil),
			colors, colors == 1 ? "" : "s",
			frames == 2 ? "2 frames, " : frames == 3 ? "3 frames, " : "",
			zoom);
		SetWindowText(hStatus, buf);
		SendMessage(hStatus, SB_SETTIPTEXT, 0, (LPARAM) buf);
	}
}

static int Fit(int dest_width, int dest_height)
{
	int width = RECOIL_GetWidth(recoil);
	int height = RECOIL_GetHeight(recoil);
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

static void ShowStatusBar(bool show)
{
	SetWindowLong(hStatus, GWL_STYLE, show ? WS_VISIBLE | WS_CHILD : WS_CHILD);
}

static int GetStatusBarHeight(void)
{
	RECT rect;
	if ((GetWindowLong(hStatus, GWL_STYLE) & WS_VISIBLE) == 0)
		return 0;
	if (!GetWindowRect(hStatus, &rect))
		return 0;
	return rect.bottom - rect.top;
}

static void CalculateWindowSize(void)
{
	RECT rect = { 0, 0, show_width, show_height };
	AdjustWindowRect(&rect, WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, TRUE);
	window_width = rect.right - rect.left;
	window_height = rect.bottom - rect.top;
	if (window_width < WINDOW_WIDTH_MIN)
		window_width = WINDOW_WIDTH_MIN;
	if (window_height < WINDOW_HEIGHT_MIN)
		window_height = WINDOW_HEIGHT_MIN;
	window_height += GetStatusBarHeight();
}

static void ResizeWindow(void)
{
	RECT rect;
	if (GetWindowRect(hWnd, &rect)) {
		int x = (rect.left + rect.right - window_width) >> 1;
		int y = (rect.top + rect.bottom - window_height) >> 1;
		MoveWindow(hWnd, x, y >= 0 ? y : 0, window_width, window_height, TRUE);
	}
}

static bool Repaint(bool fit_to_desktop)
{
	if (image_loaded) {
		if (fullscreen)
			Fit(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
		else {
			int desktop_width = GetSystemMetrics(SM_CXFULLSCREEN);
			int desktop_height = GetSystemMetrics(SM_CYFULLSCREEN);
			if (zoom < ZOOM_MIN)
				zoom = ZOOM_MIN;
			show_width = MulDiv(RECOIL_GetWidth(recoil), zoom, 100);
			show_height = MulDiv(RECOIL_GetHeight(recoil), zoom, 100);
			CalculateWindowSize();
			if (window_width > desktop_width || window_height > desktop_height) {
				if (!fit_to_desktop)
					return false;
				zoom = Fit(desktop_width, desktop_height);
				CalculateWindowSize();
			}
			ResizeWindow();
			RECT rect;
			GetClientRect(hWnd, &rect);
			rect.bottom -= GetStatusBarHeight();
			if (rect.bottom < show_height) {
				window_height += show_height - rect.bottom;
				ResizeWindow();
			}
		}
	}
	InvalidateRect(hWnd, NULL, TRUE);
	UpdateText();

	UINT check;
	switch (zoom) {
	case 100:
		check = IDM_ZOOM1;
		break;
	case 200:
		check = IDM_ZOOM2;
		break;
	case 300:
		check = IDM_ZOOM3;
		break;
	case 400:
		check = IDM_ZOOM4;
		break;
	case 500:
		check = IDM_ZOOM5;
		break;
	case 600:
		check = IDM_ZOOM6;
		break;
	case 700:
		check = IDM_ZOOM7;
		break;
	case 800:
		check = IDM_ZOOM8;
		break;
	case 900:
		check = IDM_ZOOM9;
		break;
	default:
		check = 0;
		break;
	}
	CheckMenuRadioItem(hMenu, IDM_ZOOM1, IDM_ZOOM9, check, MF_BYCOMMAND);

	return true;
}

static void ToggleFullscreen(void)
{
	if (fullscreen) {
		ShowCursor(TRUE);
		ShowStatusBar(status_bar);
		SetWindowLong(hWnd, GWL_STYLE, WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);
		SetMenu(hWnd, hMenu);
		fullscreen = false;
	}
	else {
		HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi = { sizeof(MONITORINFO) };
		if (GetMonitorInfo(hMon, &mi)) {
			ShowCursor(FALSE);
			ShowStatusBar(false);
			SetWindowLong(hWnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);
			SetMenu(hWnd, NULL);
			MoveWindow(hWnd, mi.rcMonitor.left, mi.rcMonitor.top,
				mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top, TRUE);
			fullscreen = true;
		}
	}
	Repaint(true);
}

static void ZoomIn(void)
{
	if (fullscreen)
		return;
	zoom += ZOOM_STEP;
	Repaint(true);
}

static void ZoomOut(void)
{
	if (fullscreen)
		return;
	do {
		zoom -= zoom % ZOOM_STEP + ZOOM_STEP;
		if (zoom < ZOOM_MIN) {
			zoom = ZOOM_MIN;
			Repaint(true);
			break;
		}
	} while (!Repaint(false));
}

static void ZoomSet(int value)
{
	if (fullscreen)
		return;
	zoom = value;
	Repaint(true);
}

static bool OpenImage(bool show_error)
{
	SetMenuEnabled(IDM_PREVFILE, true);
	SetMenuEnabled(IDM_NEXTFILE, true);
	SetMenuEnabled(IDM_FIRSTFILE, true);
	SetMenuEnabled(IDM_LASTFILE, true);

	static BYTE content[RECOIL_MAX_CONTENT_LENGTH];
	int content_len = RECOILWin32_SlurpFile(image_filename, content, sizeof(content));
	if (content_len < 0) {
		if (show_error)
			ShowError("Cannot open file");
		return false;
	}

	image_loaded = RECOILWin32_Decode(recoil, image_filename, content, content_len);
	SetMenuEnabled(IDM_SAVEAS, image_loaded);
	SetMenuEnabled(IDM_COPY, image_loaded);
	SetMenuEnabled(IDM_FULLSCREEN, image_loaded);
	SetMenuEnabled(IDM_ZOOMIN, image_loaded);
	SetMenuEnabled(IDM_ZOOMOUT, image_loaded);
	for (int id = IDM_ZOOM1; id <= IDM_ZOOM9; id++)
		SetMenuEnabled(id, image_loaded);
	if (!image_loaded) {
		SetMenuEnabled(IDM_INVERT, false);
		SetWindowText(hWnd, _T(APP_TITLE));
		SetWindowText(hStatus, NULL);
		if (show_error) {
			Repaint(true);
			ShowError("Decoding error");
		}
		return false;
	}
	int width = RECOIL_GetWidth(recoil);
	int height = RECOIL_GetHeight(recoil);
	const int *palette = RECOIL_ToPalette(recoil, bitmap.pixels + RECOIL_MAX_PIXELS_LENGTH); // an area we won't need later
	int colors = RECOIL_GetColors(recoil);
	SetMenuEnabled(IDM_INVERT, colors == 2);

	bitmap.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmap.bmiHeader.biWidth = width;
	bitmap.bmiHeader.biHeight = height;
	bitmap.bmiHeader.biPlanes = 1;
	bitmap.bmiHeader.biBitCount = palette != NULL ? 8 : 24;
	bitmap.bmiHeader.biCompression = BI_RGB;
	bitmap.bmiHeader.biXPelsPerMeter = RECOIL_GetXPixelsPerMeter(recoil);
	bitmap.bmiHeader.biYPelsPerMeter = RECOIL_GetYPixelsPerMeter(recoil);
	if (bitmap.bmiHeader.biXPelsPerMeter == 0)
		bitmap.bmiHeader.biXPelsPerMeter = bitmap.bmiHeader.biYPelsPerMeter = 96 * 10000 / 254;
	if (palette != NULL) {
		int bytesPerLine = (width + 3) & ~3;
		bitmap.bmiHeader.biSizeImage = sizeof(BITMAPINFOHEADER) + colors * sizeof(RGBQUAD) + height * bytesPerLine;
		bitmap.bmiHeader.biClrUsed = colors;
		bitmap.bmiHeader.biClrImportant = colors;
		memcpy(bitmap.bmiColors, palette, colors * 4);
		bitmap_pixels = (BYTE *) (bitmap.bmiColors + colors);
		for (int y = 0; y < height; y++)
			memcpy(bitmap_pixels + (height - 1 - y) * bytesPerLine, bitmap.pixels + RECOIL_MAX_PIXELS_LENGTH + y * width, width);
	}
	else {
		int bytesPerLine = (width * 3 + 3) & ~3;
		const int *pixels = RECOIL_GetPixels(recoil);
		bitmap.bmiHeader.biSizeImage = sizeof(BITMAPINFOHEADER) + height * bytesPerLine;
		bitmap.bmiHeader.biClrUsed = 0;
		bitmap.bmiHeader.biClrImportant = 0;
		bitmap_pixels = (BYTE *) bitmap.bmiColors;
		for (int y = 0; y < height; y++) {
			BYTE *dest = bitmap_pixels + (height - 1 - y) * bytesPerLine;
			for (int x = 0; x < width; x++) {
				int rgb = *pixels++;
				*dest++ = (BYTE) rgb;
				*dest++ = (BYTE) (rgb >> 8);
				*dest++ = (BYTE) (rgb >> 16);
			}
		}
	}

	Repaint(true);
	return true;
}

static void SelectAndOpenImage(void)
{
	static OPENFILENAME ofn = {
		sizeof(OPENFILENAME),
		NULL,
		0,
		_T(RECOILWIN_FILTERS),
		NULL,
		0,
		1,
		image_filename,
		MAX_PATH,
		NULL,
		0,
		NULL,
		_T("Select picture"),
		OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
		0,
		0,
		NULL,
		0,
		NULL,
		NULL
	};
	ofn.hwndOwner = hWnd;
	if (fullscreen)
		ShowCursor(TRUE);
	if (GetOpenFileName(&ofn))
		OpenImage(true);
	if (fullscreen)
		ShowCursor(FALSE);
}

static bool GetSiblingFile(LPTSTR filename, int dir)
{
	int path_len = GetPathLength(filename);
	if (path_len > MAX_PATH - 2)
		return false;
	_TCHAR mask[MAX_PATH];
	_stprintf(mask, MAX_PATH, _T("%.*" PRIts "*"), path_len, filename);
	_TCHAR best[MAX_PATH];
	best[0] = '\0';
	WIN32_FIND_DATA wfd;
	HANDLE fh = FindFirstFile(mask, &wfd);
	if (fh == INVALID_HANDLE_VALUE)
		return false;
	do {
		if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0
		 && wfd.nFileSizeHigh == 0 && wfd.nFileSizeLow <= RECOIL_MAX_CONTENT_LENGTH
		 && RECOILWin32_IsOurFile(wfd.cFileName)
		 && ((dir & 1) == 0 || _tcsicmp(wfd.cFileName, filename + path_len) * dir > 0)
		 && (best[0] == '\0' || (_tcsicmp(wfd.cFileName, best) ^ dir) < 0))
			_tcscpy_s(best, MAX_PATH, wfd.cFileName);
	} while (FindNextFile(fh, &wfd));
	FindClose(fh);
	if (best[0] == '\0')
		return false;
	return _tcscpy_s(filename + path_len, MAX_PATH - path_len, best) == 0;
}

static void OpenSiblingImage(int dir)
{
	if (image_filename[0] == '\0')
		return;
	while (GetSiblingFile(image_filename, dir)) {
		if (skip_files) {
			if (OpenImage(false))
				return;
			// first->next, last->prev
			if ((dir & 1) == 0)
				dir >>= 1;
		}
		else {
			OpenImage(true);
			return;
		}
	}
	if (!image_loaded)
		OpenImage(true);
}

static void SelectAndSaveImage(void)
{
	static _TCHAR png_filename[MAX_PATH] = _T("");
	static OPENFILENAME ofn = {
		sizeof(OPENFILENAME),
		NULL,
		0,
		_T("PNG images (*.png)\0*.png\0\0"),
		NULL,
		0,
		1,
		png_filename,
		MAX_PATH,
		NULL,
		0,
		NULL,
		_T("Select output file"),
		OFN_ENABLESIZING | OFN_EXPLORER | OFN_OVERWRITEPROMPT,
		0,
		0,
		_T("png"),
		0,
		NULL,
		NULL
	};
	ofn.hwndOwner = hWnd;
	if (fullscreen)
		ShowCursor(TRUE);
	if (GetSaveFileName(&ofn)) {
		FILE *fp = _tfopen(png_filename, _T("wb"));
		if (fp == NULL || !RECOIL_SavePng(recoil, fp))
			ShowError("Error writing file");
	}
	if (fullscreen)
		ShowCursor(FALSE);
}

static void SetNtsc(bool ntsc)
{
	RECOIL_SetNtsc(recoil, ntsc);
	CheckMenuRadioItem(hMenu, IDM_PAL, IDM_NTSC, ntsc ? IDM_NTSC : IDM_PAL, MF_BYCOMMAND);
	if (image_loaded)
		OpenImage(true);
}

static bool OpenPalette(LPCTSTR filename)
{
	BYTE atari8_palette[768 + 1];
	int atari8_palette_len = RECOILWin32_SlurpFile(filename, atari8_palette, sizeof(atari8_palette));
	if (atari8_palette_len < 0) {
		ShowError("Cannot open file");
		return false;
	}
	if (atari8_palette_len != 768) {
		ShowError("Invalid file length - must be 768 bytes");
		return false;
	}
	RECOIL_SetAtari8Palette(recoil, atari8_palette);
	return true;
}

static void UseExternalPalette(bool act)
{
	SetMenuEnabled(IDM_USEPALETTE, act);
	SetMenuCheck(IDM_USEPALETTE, act);
	if (image_loaded)
		OpenImage(true);
}

static void SelectAndOpenPalette(void)
{
	static _TCHAR act_filename[MAX_PATH] = _T("");
	static OPENFILENAME ofn = {
		sizeof(OPENFILENAME),
		NULL,
		0,
		_T("Palette files (*.act;*.pal)\0"
		"*.act;*pal\0"
		"\0"),
		NULL,
		0,
		1,
		act_filename,
		MAX_PATH,
		NULL,
		0,
		NULL,
		_T("Select Atari 8-bit palette"),
		OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
		0,
		0,
		NULL,
		0,
		NULL,
		NULL
	};
	ofn.hwndOwner = hWnd;
	if (fullscreen)
		ShowCursor(TRUE);
	if (GetOpenFileName(&ofn) && OpenPalette(act_filename))
		UseExternalPalette(true);
	if (fullscreen)
		ShowCursor(FALSE);
}

static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			if (image_loaded) {
				RECT rect;
				GetClientRect(hWnd, &rect);
				rect.bottom -= GetStatusBarHeight();
				int x = rect.right > show_width ? (rect.right - show_width) >> 1 : 0;
				int y = rect.bottom > show_height ? (rect.bottom - show_height) >> 1 : 0;
				SetStretchBltMode(hdc, COLORONCOLOR);
				StretchDIBits(hdc, x, y, show_width, show_height, 0, 0, RECOIL_GetWidth(recoil), RECOIL_GetHeight(recoil),
					bitmap_pixels, (CONST BITMAPINFO *) &bitmap, DIB_RGB_COLORS, SRCCOPY);
			}
			EndPaint(hWnd, &ps);
		}
		break;
	case WM_SIZE:
		SendMessage(hStatus, WM_SIZE, 0, 0);
		break;
	case WM_DPICHANGED:
		Repaint(true);
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
		switch (LOWORD(wParam)) {
		case IDM_OPEN:
			SelectAndOpenImage();
			break;
		case IDM_PREVFILE:
			OpenSiblingImage(-1);
			break;
		case IDM_NEXTFILE:
			OpenSiblingImage(1);
			break;
		case IDM_FIRSTFILE:
			OpenSiblingImage(2);
			break;
		case IDM_LASTFILE:
			OpenSiblingImage(-2);
			break;
		case IDM_SKIPFILES:
			skip_files = !skip_files;
			SetMenuCheck(IDM_SKIPFILES, skip_files);
			break;
		case IDM_SAVEAS:
			SelectAndSaveImage();
			break;
		case IDM_EXIT:
			PostQuitMessage(0);
			break;
		case IDM_COPY:
			if (OpenClipboard(hWnd)) {
				void *p = (void *) GlobalAlloc(GMEM_FIXED, bitmap.bmiHeader.biSizeImage);
				if (p != NULL) {
					memcpy(p, &bitmap, bitmap.bmiHeader.biSizeImage);
					EmptyClipboard();
					SetClipboardData(CF_DIB, GlobalHandle(p));
				}
				CloseClipboard();
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
		case IDM_ZOOM1:
			ZoomSet(100);
			break;
		case IDM_ZOOM2:
			ZoomSet(200);
			break;
		case IDM_ZOOM3:
			ZoomSet(300);
			break;
		case IDM_ZOOM4:
			ZoomSet(400);
			break;
		case IDM_ZOOM5:
			ZoomSet(500);
			break;
		case IDM_ZOOM6:
			ZoomSet(600);
			break;
		case IDM_ZOOM7:
			ZoomSet(700);
			break;
		case IDM_ZOOM8:
			ZoomSet(800);
			break;
		case IDM_ZOOM9:
			ZoomSet(900);
			break;
		case IDM_INVERT:
			if (bitmap.bmiHeader.biClrUsed == 2) {
				RGBQUAD c0 = bitmap.bmiColors[0];
				bitmap.bmiColors[0] = bitmap.bmiColors[1];
				bitmap.bmiColors[1] = c0;
				Repaint(true);
			}
			break;
		case IDM_PAL:
			SetNtsc(false);
			break;
		case IDM_NTSC:
			SetNtsc(true);
			break;
		case IDM_LOADPALETTE:
			SelectAndOpenPalette();
			break;
		case IDM_USEPALETTE:
			RECOIL_SetAtari8Palette(recoil, NULL);
			UseExternalPalette(false);
			break;
		case IDM_SHOWPATH:
			show_path = !show_path;
			SetMenuCheck(IDM_SHOWPATH, show_path);
			UpdateText();
			break;
		case IDM_STATUSBAR:
			status_bar = !status_bar;
			SetMenuCheck(IDM_STATUSBAR, status_bar);
			ShowStatusBar(status_bar);
			Repaint(true);
			break;
		case IDM_ABOUT:
			ShowAbout();
			break;
		default:
			break;
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_COPYDATA:
		{
			PCOPYDATASTRUCT pcds = (PCOPYDATASTRUCT) lParam;
			if (pcds->dwData == 'O' && pcds->cbData <= sizeof(image_filename)) {
				memcpy(image_filename, pcds->lpData, pcds->cbData);
				OpenImage(true);
			}
		}
		break;
	case WM_DROPFILES:
		{
			HDROP hDrop = (HDROP) wParam;
			// when dragging many files, get just the first filename
			DragQueryFile(hDrop, 0, image_filename, MAX_PATH);
			DragFinish(hDrop);
			OpenImage(true);
		}
		return 0;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	int argc;
	LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	LPWSTR filename = argc > 1 ? argv[1] : NULL;

	hWnd = FindWindow(WND_CLASS_NAME, NULL);
	if (hWnd != NULL) {
		// an instance of RECOILWin is already running
		if (filename != NULL) {
			// pass the filename
			COPYDATASTRUCT cds = { 'O', ((DWORD) wcslen(filename) + 1) * sizeof(WCHAR), filename };
			SendMessage(hWnd, WM_COPYDATA, (WPARAM) NULL, (LPARAM) &cds);
		}
		else {
			// bring the open dialog to top
			HWND hChild = GetLastActivePopup(hWnd);
			if (hChild != hWnd)
				SetForegroundWindow(hChild);
		}
		return 0;
	}

	recoil = RECOIL_New();
	if (recoil == NULL)
		return 1;

	hInst = hInstance;
	static const INITCOMMONCONTROLSEX iccx = { sizeof(INITCOMMONCONTROLSEX), ICC_BAR_CLASSES };
	InitCommonControlsEx(&iccx);
	WNDCLASS wc;
	wc.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU);
	wc.lpszClassName = WND_CLASS_NAME;
	RegisterClass(&wc);

	hWnd = CreateWindow(WND_CLASS_NAME, _T(APP_TITLE),
		WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL
	);
	hMenu = GetMenu(hWnd);
	CheckMenuRadioItem(hMenu, IDM_PAL, IDM_NTSC, IDM_PAL, MF_BYCOMMAND);

	hStatus = CreateWindow(STATUSCLASSNAME, NULL,
		WS_VISIBLE | WS_CHILD | SBT_TOOLTIPS,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		hWnd, NULL, hInstance, NULL
	);

	HACCEL hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATORS));
	DragAcceptFiles(hWnd, TRUE);

	if (filename != NULL && wcscpy_s(image_filename, sizeof(image_filename) / sizeof(image_filename[0]), filename) == 0)
		OpenImage(true);
	else
		SelectAndOpenImage();

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		if (!TranslateAccelerator(hWnd, hAccel, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return 0;
}
