/*
 * failwin.c - Windows API port of FAIL
 *
 * Copyright (C) 2009-2011  Piotr Fusik and Adrian Matoga
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
#define _WIN32_IE	0x0300
#include <commctrl.h>
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

static HINSTANCE hInst;
static HWND hWnd;
static HMENU hMenu;
static HWND hStatus;

static byte atari_palette[FAIL_PALETTE_MAX + 1];
static BOOL atari_palette_loaded = FALSE;
static BOOL use_atari_palette = FALSE;
static BOOL fullscreen = FALSE;
static int zoom = 100;
static int show_width;
static int show_height;
static int window_width;
static int window_height;
static BOOL show_path = FALSE;
static BOOL status_bar = TRUE;

static char image_filename[MAX_PATH] = "";
static BOOL image_loaded = FALSE;
static FAIL_ImageInfo image_info;
static byte pixels[FAIL_PIXELS_MAX];
static byte palette[FAIL_PALETTE_MAX];

static struct {
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD bmiColors[256];
	byte pixels[FAIL_PIXELS_MAX];
} bitmap;
static byte *bitmap_pixels;

static void ShowError(const char *message)
{
	MessageBox(hWnd, message, APP_TITLE, MB_OK | MB_ICONERROR);
}

static void ShowAbout(void)
{
	MSGBOXPARAMS mbp = {
		sizeof(MSGBOXPARAMS),
		hWnd,
		hInst,
		FAIL_CREDITS
		"FAIL icon (C) 2009 Pawel Szewczyk\n\n"
		FAIL_COPYRIGHT,
		APP_TITLE " " FAIL_VERSION,
		MB_OK | MB_USERICON,
		MAKEINTRESOURCE(IDI_APP),
		0,
		NULL,
		LANG_NEUTRAL
	};
	MessageBoxIndirect(&mbp);
}

static void SetMenuEnabled(int id, BOOL enable)
{
	EnableMenuItem(hMenu, id, MF_BYCOMMAND | (enable ? MF_ENABLED : MF_GRAYED));
}

static void SetMenuCheck(int id, BOOL check)
{
	CheckMenuItem(hMenu, id, MF_BYCOMMAND | (check ? MF_CHECKED : MF_UNCHECKED));
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

static void UpdateText(void)
{
	char buf[MAX_PATH + 32];
	const char *filename = image_filename;
	if (filename[0] == '\0')
		return;
	if (!show_path)
		filename += GetPathLength(filename);
	sprintf(buf, "%s - " APP_TITLE, filename);
	SetWindowText(hWnd, buf);
	if (image_loaded) {
		sprintf(buf, "%dx%d, %d colors, %d%%", image_info.original_width,
			image_info.original_height, image_info.colors, zoom);
		SetWindowText(hStatus, buf);
	}
}

static int Fit(int dest_width, int dest_height)
{
	if (image_info.width * dest_height < image_info.height * dest_width) {
		show_width = MulDiv(image_info.width, dest_height, image_info.height);
		show_height = dest_height;
		return MulDiv(100, dest_height, image_info.height);
	}
	else {
		show_width = dest_width;
		show_height = MulDiv(image_info.height, dest_width, image_info.width);
		return MulDiv(100, dest_width, image_info.width);
	}
}

static void ShowStatusBar(BOOL show)
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
	window_width = show_width + GetSystemMetrics(SM_CXFIXEDFRAME) * 2;
	window_height = show_height + GetSystemMetrics(SM_CYFIXEDFRAME) * 2 + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYMENU);
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
		MoveWindow(hWnd, x >= 0 ? x : 0, y >= 0 ? y : 0, window_width, window_height, TRUE);
	}
}

static BOOL Repaint(BOOL fit_to_desktop)
{
	if (image_loaded) {
		if (fullscreen)
			Fit(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
		else {
			int desktop_width = GetSystemMetrics(SM_CXFULLSCREEN);
			int desktop_height = GetSystemMetrics(SM_CYFULLSCREEN);
			RECT rect;
			show_width = MulDiv(image_info.width, zoom, 100);
			show_height = MulDiv(image_info.height, zoom, 100);
			CalculateWindowSize();
			if (window_width > desktop_width || window_height > desktop_height) {
				if (!fit_to_desktop)
					return FALSE;
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
	UpdateText();
	return TRUE;
}

static void ToggleFullscreen(void)
{
	if (fullscreen) {
		ShowCursor(TRUE);
		ShowStatusBar(status_bar);
		SetWindowLong(hWnd, GWL_STYLE, WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);
		SetMenu(hWnd, hMenu);
		fullscreen = FALSE;
	}
	else {
		ShowCursor(FALSE);
		ShowStatusBar(FALSE);
		SetWindowLong(hWnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);
		SetMenu(hWnd, NULL);
		MoveWindow(hWnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), TRUE);
		fullscreen = TRUE;
	}
	Repaint(TRUE);
}

static void ZoomIn(void)
{
	if (fullscreen)
		return;
	zoom += ZOOM_STEP;
	Repaint(TRUE);
}

static void ZoomOut(void)
{
	if (fullscreen)
		return;
	do {
		zoom -= zoom % ZOOM_STEP + ZOOM_STEP;
		if (zoom < ZOOM_MIN) {
			zoom = ZOOM_MIN;
			Repaint(TRUE);
			break;
		}
	} while (!Repaint(FALSE));
}

static BOOL LoadFile(const char *filename, byte *buffer, int *len)
{
	HANDLE fh;
	BOOL ok;
	fh = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (fh == INVALID_HANDLE_VALUE)
		return FALSE;
	ok = ReadFile(fh, buffer, *len, (LPDWORD) len, NULL);
	CloseHandle(fh);
	return ok;
}

static void CopyPaletteToBitmap(void)
{
	int i;
	for (i = 0; i < image_info.colors; i++) {
		bitmap.bmiColors[i].rgbRed = palette[3 * i];
		bitmap.bmiColors[i].rgbGreen = palette[3 * i + 1];
		bitmap.bmiColors[i].rgbBlue = palette[3 * i + 2];
	}
}

static BOOL OpenImage(BOOL show_error)
{
	byte image[FAIL_IMAGE_MAX];
	int image_len = sizeof(image);

	SetMenuEnabled(IDM_PREVFILE, TRUE);
	SetMenuEnabled(IDM_NEXTFILE, TRUE);

	if (!LoadFile(image_filename, image, &image_len)) {
		if (show_error)
			ShowError("Cannot open file");
		return FALSE;
	}

	image_loaded = FAIL_DecodeImage(image_filename, image, image_len,
		use_atari_palette ? atari_palette : NULL,
		&image_info, pixels, palette);
	SetMenuEnabled(IDM_SAVEAS, image_loaded);
	SetMenuEnabled(IDM_COPY, image_loaded);
	SetMenuEnabled(IDM_FULLSCREEN, image_loaded);
	SetMenuEnabled(IDM_ZOOMIN, image_loaded);
	SetMenuEnabled(IDM_ZOOMOUT, image_loaded);
	SetMenuEnabled(IDM_INVERT, image_loaded && image_info.colors == 2);
	if (!image_loaded) {
		SetWindowText(hWnd, APP_TITLE);
		SetWindowText(hStatus, NULL);
		if (show_error) {
			Repaint(TRUE);
			ShowError("Decoding error");
		}
		return FALSE;
	}

	bitmap.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmap.bmiHeader.biWidth = image_info.width;
	bitmap.bmiHeader.biHeight = image_info.height;
	bitmap.bmiHeader.biPlanes = 1;
	bitmap.bmiHeader.biBitCount = image_info.colors <= 256 ? 8 : 24;
	bitmap.bmiHeader.biCompression = BI_RGB;
	bitmap.bmiHeader.biXPelsPerMeter = 1000;
	bitmap.bmiHeader.biYPelsPerMeter = 1000;
	if (image_info.colors <= 256) {
		int y;
		bitmap.bmiHeader.biSizeImage =
			sizeof(BITMAPINFOHEADER) + image_info.colors * sizeof(RGBQUAD) + image_info.width * image_info.height;
		bitmap.bmiHeader.biClrUsed = image_info.colors;
		bitmap.bmiHeader.biClrImportant = image_info.colors;
		CopyPaletteToBitmap();
		bitmap_pixels = (byte *) (bitmap.bmiColors + image_info.colors);
		for (y = 0; y < image_info.height; y++)
			memcpy(bitmap_pixels + (image_info.height - 1 - y) * image_info.width,
				pixels + y * image_info.width, image_info.width);
	}
	else {
		int y;
		bitmap.bmiHeader.biSizeImage =
			sizeof(BITMAPINFOHEADER) + image_info.width * image_info.height * 3;
		bitmap.bmiHeader.biClrUsed = 0;
		bitmap.bmiHeader.biClrImportant = 0;
		bitmap_pixels = (byte *) bitmap.bmiColors;
		for (y = 0; y < image_info.height; y++) {
			const byte *p = pixels + y * image_info.width * 3;
			byte *q = bitmap_pixels + (image_info.height - 1 - y) * image_info.width * 3;
			int i;
			for (i = 0; i < image_info.width * 3; i += 3) {
				q[i] = p[i + 2];
				q[i + 1] = p[i + 1];
				q[i + 2] = p[i];
			}
		}
	}

	Repaint(TRUE);
	return TRUE;
}

static void SelectAndOpenImage(void)
{
	static OPENFILENAME ofn = {
		sizeof(OPENFILENAME),
		NULL,
		0,
		"All supported\0*.256;*.ap2;*.ap3;*.apc;*.cci;*.chr;*.cin;*.cpr;*.dgp;*.esc;*.fnt;*.fwa;*.ghg;*.gr8;*.gr9;*.hip;*.hr;*.hr2;*.ige;*.ilc;*.inp;*.int;*.ist;*.jgp;*.mbg;*.mch;*.mcp;*.mgp;*.mic;*.pic;*.plm;*.pzm;*.raw;*.rgb;*.rip;*.rm0;*.rm1;*.rm2;*.rm3;*.rm4;*.shp;*.sxs;*.tip;*.wnd;*.xlp\0"
#define FAIL_FILTER(description, masks) description " (" masks ")\0" masks "\0"
		FAIL_FILTER("Hi-res", "*.cpr;*.ghg;*.gr8;*.mbg")
		FAIL_FILTER("Using DLI", "*.fwa;*.mch;*.mgp;*.rm0;*.rm1;*.rm2;*.rm3;*.rm4")
		FAIL_FILTER("Other non-interlaced", "*.gr9;*.mic;*.pic;*.shp;*.wnd")
		FAIL_FILTER("80x96x256", "*.256;*.ap2;*.apc;*.plm")
		FAIL_FILTER("80x192x256", "*.ap3;*.dgp;*.esc;*.ilc;*.pzm")
		FAIL_FILTER("CIN", "*.cci;*.cin")
		FAIL_FILTER("HIP/RIP/TIP", "*.hip;*.rip;*.tip")
		FAIL_FILTER("Other interlaced", "*.hr;*.hr2;*.ige;*.inp;*.int;*.ist;*.mcp;*.raw;*.rgb;*.xlp")
		FAIL_FILTER("Fonts", "*.chr;*.fnt;*.jgp;*.sxs")
		"\0",
		NULL,
		0,
		1,
		image_filename,
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
	if (fullscreen)
		ShowCursor(TRUE);
	if (GetOpenFileName(&ofn))
		OpenImage(TRUE);
	if (fullscreen)
		ShowCursor(FALSE);
}

static BOOL GetSiblingFile(char *filename, int dir)
{
	int path_len;
	char mask[MAX_PATH];
	char best[MAX_PATH];
	HANDLE fh;
	WIN32_FIND_DATA wfd;
	path_len = GetPathLength(filename);
	if (path_len > MAX_PATH - 2)
		return FALSE;
	memcpy(mask, filename, path_len);
	mask[path_len] = '*';
	mask[path_len + 1] = '\0';
	best[0] = '\0';
	fh = FindFirstFile(mask, &wfd);
	if (fh == INVALID_HANDLE_VALUE)
		return FALSE;
	do {
		if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0
			&& wfd.nFileSizeHigh == 0
			&& wfd.nFileSizeLow <= FAIL_IMAGE_MAX
			&& FAIL_IsOurFile(wfd.cFileName)
			&& _stricmp(wfd.cFileName, filename + path_len) * dir > 0) {
			if (best[0] == '\0'
			 || _stricmp(wfd.cFileName, best) * dir < 0) {
				strcpy(best, wfd.cFileName);
			}
		}
	} while (FindNextFile(fh, &wfd));
	FindClose(fh);
	if (best[0] == '\0')
		return FALSE;
	if (path_len + strlen(best) + 1 >= MAX_PATH)
		return FALSE;
	strcpy(filename + path_len, best);
	return TRUE;
}

static void OpenSiblingImage(int dir)
{
	if (image_filename[0] == '\0')
		return;
	while (GetSiblingFile(image_filename, dir)) {
		if (OpenImage(FALSE))
			return;
	}
	if (!image_loaded)
		OpenImage(TRUE);
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
	if (fullscreen)
		ShowCursor(TRUE);
	if (GetSaveFileName(&ofn)) {
		if (!PNG_Save(png_filename, image_info.width, image_info.height, image_info.colors, pixels, palette))
			ShowError("Error writing file");
	}
	if (fullscreen)
		ShowCursor(FALSE);
}

static BOOL OpenPalette(const char *filename)
{
	int atari_palette_len = sizeof(atari_palette);
	if (!LoadFile(filename, atari_palette, &atari_palette_len)) {
		ShowError("Cannot open file");
		return FALSE;
	}
	if (atari_palette_len != FAIL_PALETTE_MAX) {
		ShowError("Invalid file length - must be 768 bytes");
		return FALSE;
	}
	return TRUE;
}

static void UseExternalPalette(BOOL act)
{
	use_atari_palette = act;
	SetMenuCheck(IDM_USEPALETTE, act);
	if (image_loaded)
		OpenImage(TRUE);
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
	if (fullscreen)
		ShowCursor(TRUE);
	if (GetOpenFileName(&ofn)) {
		atari_palette_loaded = OpenPalette(act_filename);
		SetMenuEnabled(IDM_USEPALETTE, atari_palette_loaded);
		UseExternalPalette(atari_palette_loaded);
	}
	if (fullscreen)
		ShowCursor(FALSE);
}

static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc;
			hdc = BeginPaint(hWnd, &ps);
			if (image_loaded) {
				RECT rect;
				int x;
				int y;
				GetClientRect(hWnd, &rect);
				rect.bottom -= GetStatusBarHeight();
				x = rect.right > show_width ? (rect.right - show_width) >> 1 : 0;
				y = rect.bottom > show_height ? (rect.bottom - show_height) >> 1 : 0;
				StretchDIBits(hdc, x, y, show_width, show_height, 0, 0, image_info.width, image_info.height,
					bitmap_pixels, (CONST BITMAPINFO *) &bitmap, DIB_RGB_COLORS, SRCCOPY);
			}
			EndPaint(hWnd, &ps);
		}
		break;
	case WM_SIZE:
		SendMessage(hStatus, WM_SIZE, 0, 0);
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
		case IDM_SAVEAS:
			SelectAndSaveImage();
			break;
		case IDM_LOADPALETTE:
			SelectAndOpenPalette();
			break;
		case IDM_USEPALETTE:
			UseExternalPalette(!use_atari_palette);
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
		case IDM_INVERT:
			if (image_info.colors == 2) {
				int i;
				for (i = 0; i < 3; i++) {
					byte t = palette[i];
					palette[i] = palette[3 + i];
					palette[3 + i] = t;
				}
				CopyPaletteToBitmap();
				Repaint(TRUE);
			}
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
			Repaint(TRUE);
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
				OpenImage(TRUE);
			}
		}
		break;
	case WM_DROPFILES:
		{
			HDROP hDrop = (HDROP) wParam;
			/* when dragging many files, get just the first filename */
			DragQueryFile(hDrop, 0, image_filename, MAX_PATH);
			DragFinish(hDrop);
			OpenImage(TRUE);
		}
		return 0;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	char *pb;
	char *pe;
	static INITCOMMONCONTROLSEX iccx = { sizeof(INITCOMMONCONTROLSEX), ICC_BAR_CLASSES };
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

	hInst = hInstance;
	InitCommonControlsEx(&iccx);
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

	hWnd = CreateWindow(WND_CLASS_NAME,
		APP_TITLE,
		WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
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

	hStatus = CreateWindow(STATUSCLASSNAME,
		NULL,
		WS_VISIBLE | WS_CHILD,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		hWnd,
		NULL,
		hInstance,
		NULL
	);

	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATORS));

	DragAcceptFiles(hWnd, TRUE);

	if (*pb != '\0') {
		memcpy(image_filename, pb, pe + 1 - pb);
		OpenImage(TRUE);
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
