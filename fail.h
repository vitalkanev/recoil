/*
 * fail.h - FAIL library interface
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

#ifndef _FAIL_H_
#define _FAIL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Version. */
#define FAIL_VERSION_MAJOR   1
#define FAIL_VERSION_MINOR   0
#define FAIL_VERSION_MICRO   2
#define FAIL_VERSION         "1.0.2"

/* Credits and copyright. */
#define FAIL_YEARS           "2009-2011"
#define FAIL_CREDITS \
	"First Atari Image Library (C) 2009-2011 Piotr Fusik and Adrian Matoga\n"
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
#define FAIL_IMAGE_MAX    30000
#define FAIL_WIDTH_MAX    384
#define FAIL_HEIGHT_MAX   240
#define FAIL_PALETTE_MAX  768
#define FAIL_PIXELS_MAX   (FAIL_WIDTH_MAX * FAIL_HEIGHT_MAX * 3)

/* Structure holding information on converted image.
   See FAIL_DecodeImage for details. */
typedef struct {
	int width;
	int height;
	int colors;
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

/* Decodes Atari image data to bitmap data with an optional palette.
    * filename (in) - name of file to decode (only extension is processed,
	                  in order to determine the image format)
    * image (in) - binary data to decode (file contents)
    * image_len (in) - number of binary data bytes
    * atari_palette (in, optional) - 768-byte array containing triplets
	                                 of bytes (R, G, B) for Atari palette colors
	                                 (NULL for built-in palette jakub.act)
    * image_info (out):
        * width - converted image width
        * height - converted image height
        * colors - exact number of colors in converted image (determines format
                   of pixels and palette)
		* original_width - original image width (informational)
		* original_height - original image height (informational)
    * pixels (out, optional) - width*height pixels, top-down, left-to-right
    * palette (out, optional) - if colors <= 256, this array is filled
	                            with 256 triplets of bytes (R, G, B)
   Returns TRUE if conversion successful, FALSE otherwise.
   After successful execution, arrays pixels and palette are formatted as follows:
    * If palette is not NULL and colors <= 256: pixels contains one byte per pixel
	  and palette has 256 entries (768 bytes) padded with blacks if necessary.
    * Otherwise pixels contains triplets of bytes (R, G, B) for consecutive pixels
	  and palette is not set. 
   After unsuccessful execution, the contents of 'out' parameters is undefined. */
abool FAIL_DecodeImage(const char *filename,
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[], byte palette[]);

#ifdef __cplusplus
}
#endif

#endif
