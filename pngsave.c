/*
 * pngsave.c - save PNG file
 *
 * Copyright (C) 2009-2021  Piotr Fusik
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

static void rgb2png(png_colorp dest, const int *src, int length)
{
	for (int i = 0; i < length; i++) {
		int rgb = src[i];
		dest[i].red = (png_byte) (rgb >> 16);
		dest[i].green = (png_byte) (rgb >> 8);
		dest[i].blue = (png_byte) rgb;
	}
}

static bool save_png_rows(const RECOIL *recoil, FILE *fp, int bit_depth, int color_type, png_const_colorp png_palette, int colors, png_bytepp row_pointers)
{
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL)
		return false;
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		png_destroy_write_struct(&png_ptr, NULL);
		return false;
	}
	// Set error handling.
	if (setjmp(png_jmpbuf(png_ptr))) {
		// If we get here, we had a problem writing the file
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return false;
	}
	png_init_io(png_ptr, fp);
	png_set_IHDR(png_ptr, info_ptr, RECOIL_GetWidth(recoil), RECOIL_GetHeight(recoil), bit_depth, color_type,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	if (png_palette != NULL)
		png_set_PLTE(png_ptr, info_ptr, png_palette, colors);
	int x_ppm = RECOIL_GetXPixelsPerMeter(recoil);
	if (x_ppm != 0)
		png_set_pHYs(png_ptr, info_ptr, x_ppm, RECOIL_GetYPixelsPerMeter(recoil), PNG_RESOLUTION_METER);
	png_write_info(png_ptr, info_ptr);
	if (bit_depth < 8)
		png_set_packing(png_ptr);
	png_write_image(png_ptr, row_pointers);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	return true;
}

static bool save_png(const RECOIL *recoil, FILE *fp, int bit_depth, int color_type, png_const_colorp png_palette, int colors, const void *pixels, size_t stride)
{
	int height = RECOIL_GetHeight(recoil);
	png_bytepp row_pointers = (png_bytepp) malloc(height * sizeof(png_bytep));
	if (row_pointers == NULL)
		return false;
	for (int y = 0; y < height; y++)
		row_pointers[y] = (png_bytep) pixels + y * stride;
	bool ok = save_png_rows(recoil, fp, bit_depth, color_type, png_palette, colors, row_pointers);
	free(row_pointers);
	return ok;
}

bool RECOIL_SavePng(RECOIL *recoil, FILE *fp)
{
	int width = RECOIL_GetWidth(recoil);
	const int *palette = RECOIL_ToPalette(recoil);
	bool ok;
	if (palette == NULL) {
		int pixels_length = width * RECOIL_GetHeight(recoil);
		png_colorp png_pixels = (png_colorp) malloc(pixels_length * sizeof(png_color));
		if (png_pixels == NULL) {
			fclose(fp);
			return false;
		}
		rgb2png(png_pixels, RECOIL_GetPixels(recoil), pixels_length);
		ok = save_png(recoil, fp, 8, PNG_COLOR_TYPE_RGB, NULL, 0, png_pixels, width * sizeof(png_color));
		free(png_pixels);
	}
	else {
		int colors = RECOIL_GetColors(recoil);
		int bit_depth = colors <= 2 ? 1
			: colors <= 4 ? 2
			: colors <= 16 ? 4
			: 8;
		png_color png_palette[256];
		rgb2png(png_palette, palette, colors);
		ok = save_png(recoil, fp, bit_depth, PNG_COLOR_TYPE_PALETTE, png_palette, colors, RECOIL_GetIndexes(recoil), width);
	}
	return fclose(fp) == 0 && ok;
}
