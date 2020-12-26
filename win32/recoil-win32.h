/*
 * recoil-win32.h - Win32 API subclass of RECOIL
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

#ifndef _RECOILWIN32_H_
#define _RECOILWIN32_H_

#include <windows.h>
#include <stdint.h>
#include "recoil.h"

#ifdef __cplusplus
extern "C" {
#endif

bool RECOILWin32_IsOurFileW(LPCWSTR filename);
int RECOILWin32_SlurpFileA(const char *filename, uint8_t *buffer, int len);
int RECOILWin32_SlurpFileW(LPCWSTR filename, uint8_t *buffer, int len);
bool RECOILWin32_DecodeA(RECOIL *self, const char *filename, uint8_t const *content, int contentLength);
bool RECOILWin32_DecodeW(RECOIL *self, LPCWSTR filename, uint8_t const *content, int contentLength);

#ifdef UNICODE
#define RECOILWin32_IsOurFile RECOILWin32_IsOurFileW
#define RECOILWin32_SlurpFile RECOILWin32_SlurpFileW
#define RECOILWin32_Decode RECOILWin32_DecodeW
#else
#define RECOILWin32_IsOurFile RECOIL_IsOurFile
#define RECOILWin32_SlurpFile RECOILWin32_SlurpFileA
#define RECOILWin32_Decode RECOILWin32_DecodeA
#endif

#ifdef __cplusplus
}
#endif

#endif
