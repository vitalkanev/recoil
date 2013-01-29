/*
 * failmagick.c - FAIL coder for ImageMagick
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
	/* TODO: Should we really perform checks, having only seven bytes of file? */
	return MagickTrue;
}

static Image *ReadFAILImage(const ImageInfo *image_info, ExceptionInfo *exception)
{
	unsigned char content[FAIL_MAX_CONTENT_LENGTH];
	int content_len;
	FAIL *fail;

	Image *image;
	MagickBooleanType status;
	PixelPacket *q;

	assert(image_info != (const ImageInfo*) NULL);
	assert(image_info->signature == MagickSignature);
	if (image_info->debug != MagickFalse)
		LogMagickEvent(TraceEvent, GetMagickModule(), "%s", image_info->filename);
	assert(exception != (ExceptionInfo*) NULL);
	assert(exception->signature == MagickSignature);
	image = AcquireImage(image_info);
	status = OpenBlob(image_info, image, ReadBinaryBlobMode, exception);
	if (status == MagickFalse) {
		image = DestroyImageList(image);
		return (Image*) NULL;
	}

	fail = FAIL_New();
	if (fail == NULL) {
		/* TODO? */
		return (Image*) NULL;
	}
	content_len = ReadBlob(image, FAIL_MAX_CONTENT_LENGTH, content);
	if (!FAIL_Decode(fail, image_info->filename, content, content_len)) {
		FAIL_Delete(fail);
		ThrowReaderException(CorruptImageError, "FileDecodingError");
	}

	image->depth = 8;
	if (SetImageExtent(image, FAIL_GetWidth(fail), FAIL_GetHeight(fail)) == MagickFalse) {
		FAIL_Delete(fail);
		InheritException(exception, &image->exception);
		return DestroyImageList(image);
	}

	q = QueueAuthenticPixels(image, 0, 0, image->columns, image->rows, NULL);
	if (q != NULL) {
		const int *pixels = FAIL_GetPixels(fail);
		int n = image->columns * image->rows;
		int i;
		for (i = 0; i < n; i++) {
			int rgb = pixels[i];
			q[i].red = ScaleCharToQuantum((unsigned char) (rgb >> 16));
			q[i].green = ScaleCharToQuantum((unsigned char) (rgb >> 8));
			q[i].blue = ScaleCharToQuantum((unsigned char) rgb);
		}
	}
	FAIL_Delete(fail);
	SyncAuthenticPixels(image, exception);

	CloseBlob(image);
	return GetFirstImageInList(image);
}

static const struct Format {
	const char *name;
	const char *description;
} formats[] = {
	{ "256", "80x96, 256 colors" },
	{ "4MI", "AtariTools-800 4 mono missiles" },
	{ "4PL", "AtariTools-800 4 mono players" },
	{ "4PM", "AtariTools-800 4 mono players and 4 mono missiles" },
	{ "A4R", "Anime 4ever; 80x256, 16-level grayscale, compressed" },
	{ "ACS", "AtariTools-800 4x8 font, 4 colors" },
	{ "AGP", "AtariTools-800 graphic" },
	{ "ALL", "Graph; 160x192, 5 colors" },
	{ "AP2", "80x96, 256 colors" },
	{ "AP3", "80x192, 256 colors, 2 frames" },
	{ "APC", "Any Point, Any Color; 80x96, 256 colors, 2 frames" },
	{ "APL", "Atari Player Editor; up to 16 16x48 frames, 4 colors" },
	{ "APP", "80x192, 256 colors, 2 frames, compressed" },
	{ "APV", "80x192, 256 colors, 2 frames" },
	{ "ART", "Art Director or Ascii-Art Editor or Artist by David Eaton" },
	{ "BG9", "160x192, 16-level grayscale" },
	{ "BKG", "Movie Maker background; 160x96, 4 colors" },
	{ "CA1", "CrackArt; 320x200, 16 colors, compressed" },
	{ "CA2", "CrackArt; 640x200, 4 colors, compressed" },
	{ "CA3", "CrackArt; 640x400, mono, compressed" },
	{ "CCI", "Champions' Interlace; 160x192, 2 frames, compressed" },
	{ "CHR", "Blazing Paddles font; mono" },
	{ "CIN", "Champions' Interlace; 160x192 or 160x200, 2 frames" },
	{ "CPR", "Trzmiel; 320x192, mono, compressed" },
	{ "DC1", "DuneGraph; 320x200, 256 colors, compressed" },
	{ "DEL", "DelmPaint; 320x240, 256 colors, compressed" },
	{ "DG1", "DuneGraph; 320x200, 256 colors" },
	{ "DGC", "DuneGraph; 320x200, 256 colors, compressed" },
	{ "DGP", "DigiPaint; 80x192, 256 colors, 2 frames" },
	{ "DGU", "DuneGraph; 320x200, 256 colors" },
	{ "DLM", "Dir Logo Maker; 88x128, mono" },
	{ "DOO", "Doodle; 640x400, mono" },
	{ "DPH", "DelmPaint; 640x480, 256 colors, compressed" },
	{ "DRG", "Atari CAD; 320x160, mono" },
	{ "ESC", "EscalPaint; 80x192, 256 colors, 2 frames" },
	{ "FNT", "Standard 8x8 font, mono" },
	{ "FTC", "Falcon True Color; 384x240, 65536 colors" },
	{ "FWA", "Fun with Art; 160x192, 128 colors" },
	{ "G09", "160x192, 16-level grayscale" },
	{ "G10", "Graphics 10; up to 80x240, 9 colors" },
	{ "G11", "Graphics 11; up to 80x240, 16 colors" },
	{ "GFB", "DeskPic" },
	{ "GHG", "Gephard Hires Graphics; up to 320x200, mono" },
	{ "GOD", "GodPaint; 65536 colors" },
	{ "GR7", "Graphics 7; up to 160x120, 4 colors" },
	{ "GR8", "Graphics 8; up to 320x240, mono" },
	{ "GR9", "Graphics 9; up to 80x240, 16-level grayscale" },
	{ "HIP", "Hard Interlace Picture; 160x200, grayscale, 2 frames" },
	{ "HPM", "Grass' Slideshow; 160x192, 4 colors, compressed" },
	{ "HR", "256x239, 3 colors, 2 frames" },
	{ "HR2", "320x200, 5 colors, 2 frames" },
	{ "IGE", "Interlace Graphics Editor; 128x96, 16 colors, 2 frames" },
	{ "ILC", "80x192, 256 colors, 2 frames" },
	{ "ING", "ING 15; 160x200, 7 colors, 2 frames" },
	{ "INP", "160x200, 7 colors, 2 frames" },
	{ "INT", "INT95a; up to 160x239, 16 colors, 2 frames" },
	{ "IPC", "ICE PCIN; 160x192, 35 colors, 2 frames" },
	{ "IST", "Interlace Studio; 160x200, 2 frames" },
	{ "JGP", "Jet Graphics Planner; 8x16 tiles, 4 colors" },
	{ "MAX", "XL-Paint MAX; 160x192, 2 frames, compressed" },
	{ "MBG", "Mad Designer; 512x256, mono" },
	{ "MCH", "Graph2Font; up to 176x240, 128 colors" },
	{ "MCP", "McPainter; 160x200, 16 colors, 2 frames" },
	{ "MCS", "160x192, 9 colors" },
	{ "MIC", "Micro Illustrator; up to 160x240, 4 colors" },
	{ "MIS", "AtariTools-800 missile; 2x240, mono" },
	{ "NEO", "NEOchrome" },
	{ "NLQ", "Daisy-Dot; 19x16 font, mono" },
	{ "PAC", "STAD; 640x400, mono, compressed" },
	{ "PC1", "DEGAS Elite; 320x200, 16 colors, compressed" },
	{ "PC2", "DEGAS Elite; 640x200, 4 colors, compressed" },
	{ "PC3", "DEGAS Elite; 640x400, mono, compressed" },
	{ "PCS", "PhotoChrome; 320x199, 1 or 2 frames, compressed" },
	{ "PGC", "Atari Portfolio; 240x64, mono, compressed" },
	{ "PGF", "Atari Portfolio; 240x64, mono" },
	{ "PI1", "DEGAS; up to 416x560, 16 colors" },
	{ "PI2", "DEGAS; 640x200, 4 colors" },
	{ "PI3", "DEGAS; 640x400, mono" },
	{ "PI4", "Fuckpaint; 320x240 or 320x200, 256 colors" },
	{ "PI9", "Fuckpaint; 320x240 or 320x200, 256 colors" },
	{ "PIC", "Koala MicroIllustrator; 160x192, 4 colors, compressed" },
	{ "PLA", "AtariTools-800 player; 8x240, mono" },
	{ "PLM", "Plama 256; 80x96, 256 colors" },
	{ "PMD", "PMG Designer by Henryk Karpowicz" },
	{ "PZM", "EscalPaint; 80x192, 256 colors, 2 frames" },
	{ "RAW", "XL-Paint MAX; 160x192, 16 colors, 2 frames" },
	{ "RGB", "ColorViewSquash; up to 160x192; 3 frames" },
	{ "RIP", "Rocky Interlace Picture; up to 320x239, 1 or 2 frames" },
	{ "RM0", "Rambrandt; 160x96, 99 colors" },
	{ "RM1", "Rambrandt; 80x192, 256 colors" },
	{ "RM2", "Rambrandt; 80x192, 104 colors" },
	{ "RM3", "Rambrandt; 80x192, 128 colors" },
	{ "RM4", "Rambrandt; 160x192, 99 colors" },
	{ "SGE", "Semi-Graphic logos Editor; 320x192, mono" },
	{ "SHC", "SAMAR Hi-res Interlace with Map of Colours; 320x192, 2 frames" },
	{ "SHP", "Blazing Paddles or Movie Maker shapes" },
	{ "SPC", "Spectrum 512 Compressed or The Graphics Magician Picture Painter" },
	{ "SPS", "Spectrum 512 Smooshed; 320x199, 512 colors, compressed" },
	{ "SPU", "Spectrum 512; 320x199, 512 colors" },
	{ "SXS", "16x16 font, mono" },
	{ "TIP", "Taquart Interlace Picture; up to 160x119, 2 frames" },
	{ "TN1", "Tiny Stuff; 320x200, 16 colors, compressed" },
	{ "TN2", "Tiny Stuff; 640x200, 4 colors, compressed" },
	{ "TN3", "Tiny Stuff; 640x400, mono, compressed" },
	{ "TNY", "Tiny Stuff; compressed" },
	{ "TRP", "EggPaint; 65536 colors" },
	{ "TRU", "IndyPaint; 65536 colors" },
	{ "WND", "Blazing Paddles window; up to 160x192, 4 colors" },
	{ "XLP", "XL-Paint; 160x192 or 160x200, 7 colors, 2 frames, compressed" }
};

/* Workaround for MagickCore.h: it omits __declspec(dllexport) for MinGW.
   As a result, no symbol has __declspec(dllexport) and thus all are exported from the DLL. */
#ifdef _WIN32
#undef ModuleExport
#define ModuleExport __declspec(dllexport)
#endif

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
