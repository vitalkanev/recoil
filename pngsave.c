/*
 * pngsave.c - save PNG file
 *
 * Copyright (C) 2009-2021  Piotr Fusik and Adrian Matoga
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

#include <stdlib.h>
#include <png.h>

#include "pngsave.h"

static void RECOIL_Rgb2Png(png_colorp dest, const int *src, int length)
{
	for (int i = 0; i < length; i++) {
		int rgb = src[i];
		dest[i].red = (png_byte) (rgb >> 16);
		dest[i].green = (png_byte) (rgb >> 8);
		dest[i].blue = (png_byte) rgb;
	}
}

bool RECOIL_SavePng(RECOIL *self, FILE *fp)
{
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		fclose(fp);
		return false;
	}
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		fclose(fp);
		png_destroy_write_struct(&png_ptr, NULL);
		return false;
	}
	/* Set error handling. */
	png_bytep pixels = NULL;
	if (setjmp(png_jmpbuf(png_ptr))) {
		/* If we get here, we had a problem writing the file */
		png_destroy_write_struct(&png_ptr, &info_ptr);
		free(pixels);
		fclose(fp);
		return false;
	}
	png_init_io(png_ptr, fp);

	int width = RECOIL_GetWidth(self);
	int height = RECOIL_GetHeight(self);
	pixels = (png_bytep) malloc(width * height);
	if (pixels == NULL) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);
		return false;
	}
	const int *palette = RECOIL_ToPalette(self, pixels);
	bool packing;
	if (palette == NULL) {
		free(pixels);
		pixels = (png_bytep) malloc(width * height * sizeof(png_color));
		if (pixels == NULL) {
			png_destroy_write_struct(&png_ptr, &info_ptr);
			fclose(fp);
			return false;
		}
		RECOIL_Rgb2Png((png_colorp) pixels, RECOIL_GetPixels(self), width * height);
		png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB,
			PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
		width *= sizeof(png_color);
		packing = false;
	}
	else {
		int colors = RECOIL_GetColors(self);
		int bit_depth = colors <= 2 ? 1
			: colors <= 4 ? 2
			: colors <= 16 ? 4
			: 8;
		png_color png_palette[256];
		RECOIL_Rgb2Png(png_palette, palette, colors);
		png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, PNG_COLOR_TYPE_PALETTE,
			PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
		png_set_PLTE(png_ptr, info_ptr, png_palette, colors);
		packing = bit_depth < 8;
	}
	float x_ppm = RECOIL_GetXPixelsPerMeter(self);
	if (x_ppm != 0) {
		float y_ppm = RECOIL_GetYPixelsPerMeter(self);
		png_set_pHYs(png_ptr, info_ptr, x_ppm, y_ppm, PNG_RESOLUTION_METER);
	}
	png_write_info(png_ptr, info_ptr);
	if (packing)
		png_set_packing(png_ptr);
	png_bytep *row_pointers = (png_bytep *) malloc(height * sizeof(png_bytep));
	for (int y = 0; y < height; y++)
		row_pointers[y] = pixels + y * width;
	png_write_image(png_ptr, row_pointers);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	free(row_pointers);
	free(pixels);
	return fclose(fp) == 0;
}
