/*
 * recoil-win32.c - Win32 API subclass of RECOIL
 *
 * Copyright (C) 2015-2020  Piotr Fusik
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

#include "recoil-win32.h"

static int RECOILWin32_ReadAndClose(HANDLE fh, uint8_t *buffer, int len)
{
	if (fh == INVALID_HANDLE_VALUE)
		return -1;
	BOOL ok = ReadFile(fh, buffer, len, (LPDWORD) &len, NULL);
	CloseHandle(fh);
	return ok ? len : -1;
}

int RECOILWin32_SlurpFileA(const char *filename, uint8_t *buffer, int len)
{
	HANDLE fh = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	return RECOILWin32_ReadAndClose(fh, buffer, len);
}

int RECOILWin32_SlurpFileW(LPCWSTR filename, uint8_t *buffer, int len)
{
	HANDLE fh = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	return RECOILWin32_ReadAndClose(fh, buffer, len);
}

typedef struct {
	int (*readFile)(const RECOIL *self, const char *filename, uint8_t *content, int contentLength);
} RECOILVtbl;

static int RECOILWin32_ReadFileA(const RECOIL *self, const char *filename, uint8_t *content, int contentLength)
{
	return RECOILWin32_SlurpFileA(filename, content, contentLength);
}

static int RECOILWin32_ReadFileW(const RECOIL *self, const char *filename, uint8_t *content, int contentLength)
{
	WCHAR wideFilename[2048];
	if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, filename, -1, wideFilename, sizeof(wideFilename) / sizeof(wideFilename[0])) <= 0)
		return -1;
	return RECOILWin32_SlurpFileW(wideFilename, content, contentLength);
}

bool RECOILWin32_DecodeA(RECOIL *self, const char *filename, uint8_t const *content, int contentLength)
{
	static const RECOILVtbl vtbl = { RECOILWin32_ReadFileA };
	*(const RECOILVtbl **) self = &vtbl;
	return RECOIL_Decode(self, filename, content, contentLength);
}

bool RECOILWin32_DecodeW(RECOIL *self, LPCWSTR filename, uint8_t const *content, int contentLength)
{
	char utf8Filename[4096];
	if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, filename, -1, utf8Filename, sizeof(utf8Filename), NULL, NULL) <= 0)
		return false;
	static const RECOILVtbl vtbl = { RECOILWin32_ReadFileW };
	*(const RECOILVtbl **) self = &vtbl;
	return RECOIL_Decode(self, utf8Filename, content, contentLength);
}
