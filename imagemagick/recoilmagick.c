/*
 * recoilmagick.c - RECOIL coder for ImageMagick
 *
 * Copyright (C) 2009-2020  Piotr Fusik and Adrian Matoga
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

#include "recoil.h"
#include "formats.h"

#ifdef MAGICK7
#include "MagickCore/studio.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/image.h"
#include "MagickCore/list.h"
#include "MagickCore/magick.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/string_.h"
#include "MagickCore/module.h"
#define MAGICK7_COMMA_EXCEPTION(exception) , exception
#else
#include "magick/studio.h"
#include "magick/blob.h"
#include "magick/blob-private.h"
#include "magick/colorspace.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/monitor-private.h"
#include "magick/quantum-private.h"
#include "magick/static.h"
#include "magick/string_.h"
#include "magick/module.h"
#define MAGICK7_COMMA_EXCEPTION(exception)
#endif

static MagickBooleanType IsRECOIL(const unsigned char *magick, const size_t length)
{
	/* TODO: Should we really perform checks, having only seven bytes of file? */
	return MagickTrue;
}

static Image *ReadRECOILImage(const ImageInfo *image_info, ExceptionInfo *exception)
{
	assert(image_info != NULL);
	assert(image_info->signature == MagickCoreSignature);
	if (image_info->debug)
		LogMagickEvent(TraceEvent, GetMagickModule(), "%s", image_info->filename);
	assert(exception != NULL);
	assert(exception->signature == MagickCoreSignature);
	Image *image = AcquireImage(image_info MAGICK7_COMMA_EXCEPTION(exception));
	if (!OpenBlob(image_info, image, ReadBinaryBlobMode, exception)) {
		(void) DestroyImageList(image);
		return NULL;
	}

	MagickSizeType content_len = GetBlobSize(image);
	if (content_len > RECOIL_MAX_CONTENT_LENGTH)
		ThrowReaderException(CorruptImageError, "ImageTypeNotSupported");
	if (content_len == 0) /* failed to get file length */
		content_len = RECOIL_MAX_CONTENT_LENGTH;
	RECOIL *recoil = RECOIL_New();
	if (recoil == NULL)
		ThrowReaderException(ResourceLimitError, "MemoryAllocationFailed");
	uint8_t *content = malloc(content_len);
	if (content == NULL) {
		RECOIL_Delete(recoil);
		ThrowReaderException(ResourceLimitError, "MemoryAllocationFailed");
	}
	content_len = ReadBlob(image, content_len, content);
	if (content_len < 0) {
		free(content);
		RECOIL_Delete(recoil);
		ThrowReaderException(CorruptImageError, "UnableToReadImageData");
	}
	if (!RECOIL_Decode(recoil, image_info->filename, content, content_len)) {
		free(content);
		RECOIL_Delete(recoil);
		ThrowReaderException(CorruptImageError, "ImageTypeNotSupported");
	}
	free(content);

	image->depth = 8;
	if (!SetImageExtent(image, RECOIL_GetWidth(recoil), RECOIL_GetHeight(recoil) MAGICK7_COMMA_EXCEPTION(exception))) {
#ifndef MAGICK7
		InheritException(exception, &image->exception);
#endif
		RECOIL_Delete(recoil);
		(void) DestroyImageList(image);
		return NULL;
	}

#ifdef MAGICK7
	Quantum
#else
	PixelPacket
#endif
		*q = QueueAuthenticPixels(image, 0, 0, image->columns, image->rows, exception);
	if (q == NULL) {
		RECOIL_Delete(recoil);
		(void) DestroyImageList(image);
		return NULL;
	}

	float x_dpi = RECOIL_GetXPixelsPerInch(recoil);
	if (x_dpi != 0) {
		image->units = PixelsPerInchResolution;
		image->resolution.x = x_dpi;
		image->resolution.y = RECOIL_GetYPixelsPerInch(recoil);;
	}

	const int *pixels = RECOIL_GetPixels(recoil);
	int num_pixels = image->columns * image->rows;
	for (int i = 0; i < num_pixels; i++) {
		int rgb = pixels[i];
		Quantum r = ScaleCharToQuantum((unsigned char) (rgb >> 16));
		Quantum g = ScaleCharToQuantum((unsigned char) (rgb >> 8));
		Quantum b = ScaleCharToQuantum((unsigned char) rgb);
#ifdef MAGICK7
		SetPixelRed(image, r, q);
		SetPixelGreen(image, g, q);
		SetPixelBlue(image, b, q);
		q += GetPixelChannels(image);
#else
		q[i].red = r;
		q[i].green = g;
		q[i].blue = b;
#endif
	}
	RECOIL_Delete(recoil);
	SyncAuthenticPixels(image, exception);

	CloseBlob(image);
	return GetFirstImageInList(image);
}

static const struct Format {
	const char *name;
	const char *description;
} formats[] = { MAGICK_RECOIL_FORMATS };

ModuleExport unsigned long RegisterRECOILImage(void)
{
	for (const struct Format *pf = formats; pf < formats + sizeof(formats) / sizeof(formats[0]); pf++) {
#ifdef MAGICK7
		MagickInfo *entry = AcquireMagickInfo("RECOIL", pf->name, pf->description);
#else
		MagickInfo *entry = SetMagickInfo(pf->name);
		entry->module = ConstantString("RECOIL");
		entry->description = ConstantString(pf->description);
#endif
		entry->decoder = ReadRECOILImage;
		entry->magick = IsRECOIL;
		RegisterMagickInfo(entry);
	}	
	return MagickImageCoderSignature;
}

ModuleExport void UnregisterRECOILImage(void)
{
	for (const struct Format *pf = formats; pf < formats + sizeof(formats) / sizeof(formats[0]); pf++)
		UnregisterMagickInfo(pf->name);
}
