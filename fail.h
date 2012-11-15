/*
 * fail.h - FAIL library interface
 *
 * Copyright (C) 2009-2012  Piotr Fusik and Adrian Matoga
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

#ifndef _FAIL_H_
#define _FAIL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Version. */
#define FAIL_VERSION_MAJOR   1
#define FAIL_VERSION_MINOR   4
#define FAIL_VERSION_MICRO   0
#define FAIL_VERSION         "1.4.0"

/* Credits and copyright. */
#define FAIL_YEARS           "2009-2012"
#define FAIL_CREDITS \
	"First Atari Image Library (C) 2009-2012 Piotr Fusik and Adrian Matoga\n"
#define FAIL_COPYRIGHT \
	"This program is free software; you can redistribute it and/or modify\n" \
	"it under the terms of the GNU General Public License as published\n" \
	"by the Free Software Foundation; either version 2 of the License,\n" \
	"or (at your option) any later version."

/* Handy type definitions. */
#ifndef FALSE
#define FALSE  0
#endif
#ifndef TRUE
#define TRUE   1
#endif
typedef int abool;
typedef unsigned char byte;

/* Limits. */
#define FAIL_IMAGE_MAX    200000
#define FAIL_WIDTH_MAX    640
#define FAIL_HEIGHT_MAX   560
#define FAIL_PALETTE_MAX  768
#define FAIL_PIXELS_MAX   (640 * 400 * 3)

/* Structure holding information on converted image.
   See FAIL_DecodeImage for details. */
typedef struct {
	int width;
	int height;
	int original_width;
	int original_height;
} FAIL_ImageInfo;

/* Checks whether the filename extension is supported by FAIL.
   TRUE doesn't necessarily mean that the file contents is valid for FAIL.
   This function is meant to avoid reading files which are known to be
   not supported (another criterium is the maximum file size, FAIL_IMAGE_MAX).
   Call FAIL_DecodeImage with image and image_len to make sure
   that the file can be decoded. */
abool FAIL_IsOurFile(const char *filename);

/* Decodes Atari image data to RGB bitmap.
	* filename (in) - name of file to decode (only extension is processed,
	                  in order to determine the image format)
	* image (in) - binary data to decode (file contents)
	* image_len (in) - number of binary data bytes (file length)
	* atari_palette (in, optional) - 768-byte array containing triplets
	                                 of bytes (R, G, B) for Atari 8-bit palette colors
	                                 (NULL for built-in palette jakub.act)
	* image_info (out):
		* width - converted image width
		* height - converted image height
		* original_width - original image width (informational)
		* original_height - original image height (informational)
	* pixels (out) - width*height RGB pixels
   Returns TRUE if conversion successful, FALSE otherwise.
   After successful execution, pixels contains triplets of bytes (R, G, B)
   for consecutive pixels, top-down, left-to-right.
   After unsuccessful execution, the contents of 'out' parameters is undefined. */
abool FAIL_DecodeImage(const char *filename,
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[]);

/* Calculates number of unique colors in an RGB image
   and optionally turns it into 8-bit indexed.
	* pixels (in, out) - on input, pixel_count RGB triplets
	* pixel_count (in) - number of pixels
	* palette (out, optional) - palette for the indexed mode, caller-allocated 768 bytes
   Returns number of colors in the image.
   If colors <= 256 and palette is not NULL, pixels are replaced with pixel_count
   bytes of palette indexes and palette is filled with 256 RGB triplets.
   Otherwise, pixels and palette are left unchanged.
 */
int FAIL_ToPalette(byte pixels[], int pixel_count, byte palette[]);

#ifdef __cplusplus
}
#endif

#endif
