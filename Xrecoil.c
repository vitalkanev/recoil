/*
 * Xrecoil.c - RECOIL plugin for XnView http://www.xnview.com
 *
 * Copyright (C) 2009-2018  Piotr Fusik and Adrian Matoga
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

#ifdef WIN32
#define _WIN32_WINNT 0x0500
#include <windows.h>

#define API __stdcall
#define DLL_EXPORT __declspec(dllexport)

#else

#include <stdint.h>

#define BOOL int32_t
#define DWORD uint32_t
#define INT int32_t

#define API
#define DLL_EXPORT

#define TRUE 1
#define FALSE 0

#endif

#include <stdio.h>
#include <stdlib.h>

#include "recoil-stdio.h"
#include "formats.h"

#define GFP_RGB	0
#define GFP_BGR	1

#define GFP_READ 0x0001
#define GFP_WRITE 0x0002

typedef struct {
	unsigned char red[256];
	unsigned char green[256];
	unsigned char blue[256];
} GFP_COLORMAP;

static size_t strlcpy(char *dst, const char *src, size_t size)
{
	int i;
	for (i = 0; i < size - 1 && src[i] != '\0'; i++)
		dst[i] = src[i];
	dst[i] = '\0';
	while (src[i] != '\0')
		i++;
	return i;
}

DLL_EXPORT BOOL API gfpGetPluginInfo(
	DWORD version, char *label, INT label_max_size,
	char *extension, INT extension_max_size, INT *support)
{
	if (version != 0x0002)
		return FALSE;

	strlcpy(label, "Retro Computer Image Library", label_max_size);
	strlcpy(extension, XNVIEW_RECOIL_EXTS, extension_max_size);
	*support = GFP_READ;

	return TRUE;
}

DLL_EXPORT void * API gfpLoadPictureInit(const char *filename)
{
	FILE *fp = fopen(filename, "rb");
	if (fp == NULL)
		return NULL;
	if (fseek(fp, 0, SEEK_END) != 0) {
		fclose(fp);
		return NULL;
	}
	int content_len = ftell(fp);
	if (content_len > RECOIL_MAX_CONTENT_LENGTH
	 || fseek(fp, 0, SEEK_SET) != 0) {
		fclose(fp);
		return NULL;
	}

	RECOIL *recoil = RECOILStdio_New();
	if (recoil == NULL) {
		fclose(fp);
		return NULL;
	}

	unsigned char *content = (unsigned char *) malloc(content_len);
	if (content == NULL) {
		RECOIL_Delete(recoil);
		fclose(fp);
		return NULL;
	}
	content_len = fread(content, 1, content_len, fp);
	fclose(fp);

	if (!RECOIL_Decode(recoil, filename, content, content_len)) {
		free(content);
		RECOIL_Delete(recoil);
		return NULL;
	}

	free(content);
	return recoil;
}

DLL_EXPORT BOOL API gfpLoadPictureGetInfo(
	void *ptr, INT *pictype, INT *width, INT *height,
	INT *dpi, INT *bits_per_pixel, INT *bytes_per_line,
	BOOL *has_colormap, char *label, INT label_max_size)
{
	const RECOIL *recoil = (const RECOIL *) ptr;

	*pictype = GFP_RGB;
	*width = RECOIL_GetWidth(recoil);
	*height = RECOIL_GetHeight(recoil);
	*dpi = 68;
	*bits_per_pixel = 24;
	*bytes_per_line = *width * 3;
	*has_colormap = FALSE;
	strlcpy(label, RECOIL_GetPlatform(recoil), label_max_size);

	return TRUE;
}

DLL_EXPORT BOOL API gfpLoadPictureGetLine(void *ptr, INT line, unsigned char *buffer)
{
	const RECOIL *recoil = (const RECOIL *) ptr;
	int x;
	int width = RECOIL_GetWidth(recoil);
	const int *pixels = RECOIL_GetPixels(recoil) + line * width;

	for (x = 0; x < width; x++) {
		int rgb = pixels[x];
		buffer[x * 3] = (unsigned char) (rgb >> 16);
		buffer[x * 3 + 1] = (unsigned char) (rgb >> 8);
		buffer[x * 3 + 2] = (unsigned char) rgb;
	}

	return TRUE;
}

DLL_EXPORT BOOL API gfpLoadPictureGetColormap(void *ptr, GFP_COLORMAP *cmap)
{
	return FALSE;
}

DLL_EXPORT void API gfpLoadPictureExit(void *ptr)
{
	RECOIL *recoil = (RECOIL *) ptr;
	RECOIL_Delete(recoil);
}

DLL_EXPORT BOOL API gfpSavePictureIsSupported(INT width, INT height, INT bits_per_pixel, BOOL has_colormap)
{
	return FALSE;
}

DLL_EXPORT void * API gfpSavePictureInit(
	const char *filename, INT width, INT height, INT bits_per_pixel,
	INT dpi, INT *picture_type, char *label, INT label_max_size)
{
	return NULL;
}

DLL_EXPORT BOOL API gfpSavePicturePutLine(void *ptr, INT line, const unsigned char *buffer)
{
	return FALSE;
}

DLL_EXPORT void API gfpSavePictureExit(void *ptr)
{
}

#ifdef WIN32
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	return TRUE;
}
#endif
