/*
 * Xfail.c - FAIL plugin for XnView http://www.xnview.com
 *
 * Copyright (C) 2009-2013  Piotr Fusik and Adrian Matoga
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
#include <stdlib.h>

#include "fail.h"

#define API __stdcall
#define DLL_EXPORT __declspec(dllexport)

#define GFP_RGB	0
#define GFP_BGR	1

#define GFP_READ 0x0001
#define GFP_WRITE 0x0002

typedef struct {
	unsigned char red[256];
	unsigned char green[256];
	unsigned char blue[256];
} GFP_COLORMAP;

#ifdef __cplusplus
extern "C"
{
#endif

DLL_EXPORT BOOL API gfpGetPluginInfo(DWORD version, LPSTR label, INT label_max_size, LPSTR extension, INT extension_max_size, INT *support);
DLL_EXPORT void * API gfpLoadPictureInit(LPCSTR filename);
DLL_EXPORT BOOL API gfpLoadPictureGetInfo(void * ptr, INT * pictype, INT * width, INT * height, INT * dpi, INT * bits_per_pixel, INT * bytes_per_line, BOOL * has_colormap, LPSTR label, INT label_max_size);
DLL_EXPORT BOOL API gfpLoadPictureGetLine(void * ptr, INT line, unsigned char * buffer);
DLL_EXPORT BOOL API gfpLoadPictureGetColormap(void * ptr, GFP_COLORMAP * cmap);
DLL_EXPORT void API gfpLoadPictureExit(void * ptr);
DLL_EXPORT BOOL API gfpSavePictureIsSupported(INT width, INT height, INT bits_per_pixel, BOOL has_colormap);
DLL_EXPORT void * API gfpSavePictureInit(LPCSTR filename, INT width, INT height, INT bits_per_pixel, INT dpi, INT * picture_type, LPSTR label, INT label_max_size);
DLL_EXPORT BOOL API gfpSavePicturePutLine(void * ptr, INT line, const unsigned char * buffer);
DLL_EXPORT void API gfpSavePictureExit(void * ptr);

#ifdef __cplusplus
}
#endif

#ifndef XNVIEW_FAIL_EXT
#define XNVIEW_FAIL_EXT "rip;gr8;mic;hip;tip;int;inp;apc;ap3;gr9;pic;cpr;cin;cci;fnt;sxs;hr;plm;ilc;mcp;ghg;hr2;mch;ige;256;ap2;jgp;dgp;esc;pzm;ist;raw;rgb;mgp;wnd;chr;shp;mbg;fwa;rm0;rm1;rm2;rm3;rm4;xlp;max;shc;all;app;sge;dlm;bkg;g09;bg9;apv;spc;apl;gr7;g10;g11;art;drg;agp;pla;mis;4pl;4mi;4pm;pgf;pgc;pi1;pi2;pi3;pc1;pc2;pc3;neo;doo;spu;tny;tn1;tn2;tn3;ca1;ca2;ca3;ing;pac;sps;gfb;pi4;pi9;dgu;dg1;trp;tru;god;ftc;del;dph;ipc;nlq;pmd;acs;pcs;hpm;mcs;a4r;dc1;dgc;cpt;iff;bl1;bl2;bl3;bru"
#endif

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

DLL_EXPORT BOOL API gfpGetPluginInfo(DWORD version, LPSTR label, INT label_max_size, LPSTR extension, INT extension_max_size, INT *support)
{
	if (version != 0x0002)
		return FALSE;

	strlcpy(label, "First Atari Image Library", label_max_size);
	strlcpy(extension, XNVIEW_FAIL_EXT, extension_max_size);
	*support = GFP_READ;

	return TRUE;
}

DLL_EXPORT void * API gfpLoadPictureInit(LPCSTR filename)
{
	FILE *fp;
	FAIL *fail;
	unsigned char *content;
	int content_len;

	fp = fopen(filename, "rb");
	if (fp == NULL)
		return NULL;

	fail = FAIL_New();
	if (fail == NULL) {
		fclose(fp);
		return NULL;
	}

	content = (unsigned char *) malloc(FAIL_MAX_CONTENT_LENGTH);
	if (content == NULL) {
		FAIL_Delete(fail);
		fclose(fp);
		return NULL;
	}
	content_len = fread(content, 1, FAIL_MAX_CONTENT_LENGTH, fp);

	if (!FAIL_Decode(fail, filename, content, content_len)) {
		free(content);
		FAIL_Delete(fail);
		fclose(fp);
		return NULL;
	}

	free(content);
	fclose(fp);
	return fail;
}

DLL_EXPORT BOOL API gfpLoadPictureGetInfo(
	void *ptr, INT *pictype, INT *width, INT *height,
	INT *dpi, INT *bits_per_pixel, INT *bytes_per_line,
	BOOL *has_colormap, LPSTR label, INT label_max_size)
{
	FAIL *fail = (FAIL *) ptr;

	*pictype = GFP_RGB;
	*width = FAIL_GetWidth(fail);
	*height = FAIL_GetHeight(fail);
	*dpi = 68;
	*bits_per_pixel = 24;
	*bytes_per_line = 3 * *width;
	*has_colormap = FALSE;
	strncpy(label, FAIL_GetPlatform(fail), label_max_size);

	return TRUE;
}

DLL_EXPORT BOOL API gfpLoadPictureGetLine(void *ptr, INT line, unsigned char *buffer)
{
	FAIL *fail = (FAIL *) ptr;
	int x;
	int width = FAIL_GetWidth(fail);
	const int *pixels = FAIL_GetPixels(fail) + line * width;

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
	FAIL *fail = (FAIL *) ptr;
	FAIL_Delete(fail);
}

DLL_EXPORT BOOL API gfpSavePictureIsSupported(INT width, INT height, INT bits_per_pixel, BOOL has_colormap)
{
	return FALSE;
}

DLL_EXPORT void * API gfpSavePictureInit(LPCSTR filename, INT width, INT height, INT bits_per_pixel, INT dpi, INT * picture_type, LPSTR label, INT label_max_size)
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

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	return TRUE;
}
