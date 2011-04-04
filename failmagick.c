/*
 * failmagick.c - FAIL coder for ImageMagick
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

/*
include <math.h> is a workaround for conflicting declarations:
C:\Program Files (x86)\ImageMagick-6.6.9-Q16\include\magick\magick-config.h:
	#define nearbyint(x)  ((ssize_t) ((x)+0.5))
C:\bin\MinGW\include\math.h:
	extern double __cdecl nearbyint ( double);
*/
#include <math.h>

#include "fail.h"

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

static MagickBooleanType IsFAIL(const unsigned char *magick, const size_t length)
{
	// TODO: Should we really perform checks, having only seven bytes of file?
	return MagickTrue;
}

static Image *ReadFAILImage(const ImageInfo *image_info, ExceptionInfo *exception)
{
	byte fail_image[FAIL_IMAGE_MAX];
	FAIL_ImageInfo fail_image_info;
	byte fail_pixels[FAIL_PIXELS_MAX];
	int fail_image_len;

	Image *image;
	MagickBooleanType status;
	PixelPacket *q;
	byte *p;

	assert(image_info != (const ImageInfo*) NULL);
	assert(image_info->signature == MagickSignature);
	if (image_info->debug != MagickFalse)
		LogMagickEvent(TraceEvent, GetMagickModule(), "%s",
			image_info->filename);
	assert(exception != (ExceptionInfo*) NULL);
	assert(exception->signature == MagickSignature);
	image = AcquireImage(image_info);
	status = OpenBlob(image_info, image, ReadBinaryBlobMode, exception);
	if (status == MagickFalse) {
		image = DestroyImageList(image);
		return (Image*) NULL;
    }

	fail_image_len = ReadBlob(image, FAIL_IMAGE_MAX, fail_image);
	if (!FAIL_DecodeImage(image_info->filename, fail_image, fail_image_len,
		NULL, &fail_image_info, fail_pixels, NULL)) {
		ThrowReaderException(CorruptImageError, "FileDecodingError");
	}

    image->depth = 8;

	if (SetImageExtent(image, fail_image_info.width, fail_image_info.height) == MagickFalse) {
		InheritException(exception, &image->exception);
        return DestroyImageList(image);
	}

	q = QueueAuthenticPixels(image, 0, 0, image->columns, image->rows, NULL);
	if (q != NULL) {
		int x;
		p = fail_pixels;
		for (x = image->columns * image->rows; x > 0; x--) {
			q->red = ScaleCharToQuantum(*p++);
			q->green = ScaleCharToQuantum(*p++);
			q->blue = ScaleCharToQuantum(*p++);
			q++;
		}
	}
	SyncAuthenticPixels(image, exception);

	CloseBlob(image);
	return GetFirstImageInList(image);
}

static const struct Format {
	const char *name;
	const char *description;
} formats[] = {
	{ "256", "80x96, 256 colors" },
	{ "AP2", "80x96, 256 colors" },
	{ "AP3", "80x192, 256 colors, interlaced" },
	{ "APC", "Any Point, Any Color; 80x96, 256 colors, interlaced" },
	{ "CCI", "Champions' Interlace; 160x192, compressed" },
	{ "CIN", "Champions' Interlace; 160x192" },
	{ "CPR", "Trzmiel; 320x192, mono, compressed" },
	{ "FNT", "Standard 8x8 font, mono" },
	{ "GHG", "Gephard Hires Graphics; up to 320x200, mono" },
	{ "GR8", "Standard 320x192, mono" },
	{ "GR9", "Standard 80x192, grayscale" },
	{ "HIP", "Hard Interlace Picture; 160x200, grayscale" },
	{ "HR", "256x239, 3 colors, interlaced" },
	{ "HR2", "320x200, 5 colors, interlaced" },
	{ "IGE", "Interlace Graphics Editor; 128x96, 16 colors, interlaced" },
	{ "ILC", "80x192, 256 colors, interlaced" },
	{ "INP", "160x200, 7 colors, interlaced" },
	{ "INT", "INT95a; up to 160x239, 16 colors, interlaced" },
	{ "JGP", "Jet Graphics Planner; 8x16 tiles, 4 colors" },
	{ "MCH", "Up to 192x240, 128 colors" },
	{ "MCP", "McPainter; 160x200, 16 colors, interlaced" },
	{ "MIC", "Standard 160x192, 4 colors" },
	{ "PIC", "Koala MicroIllustrator; 160x192, 4 colors, compressed" },
	{ "PLM", "Plama 256; 80x96, 256 colors" },
	{ "RIP", "Rocky Interlace Picture; up to 160x239" },
	{ "SXS", "16x16 font, mono" },
	{ "TIP", "Taquart Interlace Picture; up to 160x119" }
};

ModuleExport unsigned long RegisterFAILImage(void)
{
	const struct Format* pf;
	MagickInfo *entry;
	for (pf = formats; pf < formats + sizeof(formats) / sizeof(formats[0]); pf++) {
		entry = SetMagickInfo(pf->name);
		entry->decoder = ReadFAILImage;
		entry->magick = IsFAIL;
		entry->description = ConstantString(pf->description);
		entry->module = ConstantString("FAIL");
		RegisterMagickInfo(entry);
	}	
	return MagickImageCoderSignature;
}

ModuleExport void UnregisterFAILImage(void)
{
	const struct Format* pf;
	for (pf = formats; pf < formats + sizeof(formats) / sizeof(formats[0]); pf++)
		UnregisterMagickInfo(pf->name);
}
