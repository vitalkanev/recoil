/*
 * recoilimagine.c - RECOIL coder for Imagine
 *
 * Copyright (C) 2012-2013  Piotr Fusik and Adrian Matoga
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

#include <windows.h>
#include <malloc.h>

#include "ImagPlug.h"
#include "recoil.h"

#define VERSION_NUMBER ((RECOIL_VERSION_MAJOR<<24)|(RECOIL_VERSION_MINOR<<16)|(RECOIL_VERSION_MICRO<<8))
#define EXTS "GR8\0HIP\0MIC\0INT\0TIP\0INP\0HR\0GR9\0PIC\0CPR\0CIN\0CCI\0APC\0PLM\0AP3\0ILC\0RIP\0FNT\0SXS\0MCP\0GHG\0HR2\0MCH\0IGE\0" "256\0AP2\0JGP\0DGP\0ESC\0PZM\0IST\0RAW\0RGB\0MGP\0WND\0CHR\0SHP\0MBG\0FWA\0RM0\0RM1\0RM2\0RM3\0RM4\0XLP\0MAX\0SHC\0ALL\0APP\0SGE\0DLM\0BKG\0G09\0BG9\0APV\0SPC\0APL\0GR7\0G10\0G11\0ART\0DRG\0AGP\0PLA\0MIS\04PL\04MI\04PM\0PGF\0PGC\0PI1\0PI2\0PI3\0PC1\0PC2\0PC3\0NEO\0DOO\0SPU\0TNY\0TN1\0TN2\0TN3\0CA1\0CA2\0CA3\0ING\0PAC\0SPS\0GFB\0PI4\0PI9\0DGU\0DG1\0TRP\0TRU\0GOD\0FTC\0DEL\0DPH\0IPC\0NLQ\0PMD\0ACS\0PCS\0HPM\0MCS\0A4R\0DC1\0DGC\0CPT\0IFF\0BL1\0BL2\0BL3\0BRU\0IP2\0IMN\0ICN\0DIN\0VZI\0IRG\0IR2\0ICE\0IMG\0XIMG\0MPP\0SCR\0MC\0MG1\0MG2\0MG4\0MG8\0ATR\0IFL\0CH4\0CH6\0CH8\0LEO\0ACBM\0"

static BOOL IMAGINEAPI checkFile(IMAGINEPLUGINFILEINFOTABLE *fileInfoTable, IMAGINELOADPARAM *loadParam, int flags)
{
	return TRUE; /* TODO? */
}

static LPIMAGINEBITMAP IMAGINEAPI loadFile(IMAGINEPLUGINFILEINFOTABLE *fileInfoTable, IMAGINELOADPARAM *loadParam, int flags)
{
	const IMAGINEPLUGININTERFACE *iface = fileInfoTable->iface;
	char *filename;
	RECOIL *recoil;
	int width;
	int height;
	const int *pixels;
	LPIMAGINEBITMAP bitmap;
	IMAGINECALLBACKPARAM param;
	int y;

	if (iface == NULL)
		return NULL;

	if (iface->lpVtbl->IsUnicode()) {
		int cch = lstrlenW((LPCWSTR) loadParam->fileName) + 1;
		filename = (char *) alloca(cch * 2);
		if (filename == NULL) {
			loadParam->errorCode = IMAGINEERROR_OUTOFMEMORY;
			return NULL;
		}
		if (WideCharToMultiByte(CP_ACP, 0, (LPCWSTR) loadParam->fileName, -1, filename, cch, NULL, NULL) <= 0) {
			loadParam->errorCode = IMAGINEERROR_FILENOTFOUND;
			return NULL;
		}
	}
	else
		filename = (char *) loadParam->fileName;

	recoil = RECOIL_New();
	if (recoil == NULL) {
		loadParam->errorCode = IMAGINEERROR_OUTOFMEMORY;
		return NULL;
	}

	if (!RECOIL_Decode(recoil, filename, loadParam->buffer, loadParam->length)) {
		RECOIL_Delete(recoil);
		loadParam->errorCode = IMAGINEERROR_UNSUPPORTEDTYPE;
		return NULL;
	}
	width = RECOIL_GetWidth(recoil);
	height = RECOIL_GetHeight(recoil);
	pixels = RECOIL_GetPixels(recoil);

	bitmap = iface->lpVtbl->Create(width, height, 24, flags);
	if (bitmap == NULL) {
		RECOIL_Delete(recoil);
		loadParam->errorCode = IMAGINEERROR_OUTOFMEMORY;
		return NULL;
	}
	if ((flags & IMAGINELOADPARAM_GETINFO) != 0) {
		RECOIL_Delete(recoil);
		return bitmap;
	}

	param.dib = bitmap;
	param.param = loadParam->callback.param;
	param.overall = height - 1;
	param.message = NULL;
	for (y = 0; y < height; y++) {
		LPBYTE dest = iface->lpVtbl->GetLineBits(bitmap, y);
		int x;
		for (x = 0; x < width; x++) {
			int rgb = *pixels++;
			/* 0xRRGGBB -> 0xBB 0xGG 0xRR */
			*dest++ = (BYTE) rgb;
			*dest++ = (BYTE) (rgb >> 8);
			*dest++ = (BYTE) (rgb >> 16);
		}
		if ((flags & IMAGINELOADPARAM_CALLBACK) != 0) {
			param.current = y;
			if (!loadParam->callback.proc(&param)) {
				RECOIL_Delete(recoil);
				loadParam->errorCode = IMAGINEERROR_ABORTED;
				return bitmap;
			}
		}
	}
	RECOIL_Delete(recoil);
	return bitmap;
}

static BOOL IMAGINEAPI registerProcA(const IMAGINEPLUGININTERFACE *iface)
{
	static const IMAGINEFILEINFOITEM fileInfoItemA = {
		checkFile,
		loadFile,
		NULL,
		(LPCTSTR) "Retro Computer (RECOIL)",
		(LPCTSTR) EXTS
	};
	return iface->lpVtbl->RegisterFileType(&fileInfoItemA) != NULL;
}

static BOOL IMAGINEAPI registerProcW(const IMAGINEPLUGININTERFACE *iface)
{
	static const IMAGINEFILEINFOITEM fileInfoItemW = {
		checkFile,
		loadFile,
		NULL,
		(LPCTSTR) L"Retro Computer (RECOIL)",
		(LPCTSTR) L"" EXTS
	};
	return iface->lpVtbl->RegisterFileType(&fileInfoItemW) != NULL;
}

__declspec(dllexport) BOOL IMAGINEAPI ImaginePluginGetInfoA(IMAGINEPLUGININFOA *dest)
{
	static const IMAGINEPLUGININFOA pluginInfoA = {
		sizeof(pluginInfoA),
		registerProcA,
		VERSION_NUMBER,
		"RECOIL Plugin",
		IMAGINEPLUGININTERFACE_VERSION
	};
	*dest = pluginInfoA;
	return TRUE;
}

__declspec(dllexport) BOOL IMAGINEAPI ImaginePluginGetInfoW(IMAGINEPLUGININFOW *dest)
{
	static const IMAGINEPLUGININFOW pluginInfoW = {
		sizeof(pluginInfoW),
		registerProcW,
		VERSION_NUMBER,
		L"RECOIL Plugin",
		IMAGINEPLUGININTERFACE_VERSION
	};
	*dest = pluginInfoW;
	return TRUE;
}
