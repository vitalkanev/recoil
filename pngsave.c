/*
 * pngsave.c - save PNG file
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

#include <stdlib.h>
#include <png.h>

#include "pngsave.h"

abool PNG_Save(const char *filename,
               int width, int height, int colors,
               const byte pixels[], const byte palette[])
{
	FILE *fp;
	png_structp png_ptr;
	png_infop info_ptr;
	png_byte bit_depth;
	png_bytep *row_pointers;
	int i, bpp = palette != NULL && colors <= 256 ? 1 : 3;

	row_pointers = (png_bytep*)malloc(height * sizeof(png_bytep));
	if (row_pointers == NULL)
		return FALSE;

	fp = fopen(filename, "wb");
	if (fp == NULL)
		return FALSE;

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (png_ptr == NULL) {
		fclose(fp);
		return FALSE;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		fclose(fp);
		png_destroy_write_struct(&png_ptr, NULL);
		return FALSE;
	}

	/* Set error handling. */
	if (setjmp(png_jmpbuf(png_ptr))) {
		/* If we get here, we had a problem writing the file */
		fclose(fp);
		free(row_pointers);
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return FALSE;
	}

	png_init_io(png_ptr, fp);

	bit_depth = 8;
	if (palette != NULL) {
		if (colors <= 2)
			bit_depth = 1;
		else if (colors <= 4)
			bit_depth = 2;
		else if (colors <= 16)
			bit_depth = 4;
	}
	
	png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth,
				 palette != NULL && colors <= 256 ?
					PNG_COLOR_TYPE_PALETTE : PNG_COLOR_TYPE_RGB,
				 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
				 PNG_FILTER_TYPE_BASE);

	if (palette != NULL && colors <= 256)
		png_set_PLTE(png_ptr, info_ptr, (png_colorp)palette, colors);

	png_write_info(png_ptr, info_ptr);

	if (bit_depth < 8)
		png_set_packing(png_ptr);

	for (i = 0; i < height; i++)
		row_pointers[i] = (png_bytep)(pixels + i*width*bpp);

	png_write_image(png_ptr, row_pointers);

	free(row_pointers);

	png_write_end(png_ptr, info_ptr);

	png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose(fp);

	return TRUE;
}
