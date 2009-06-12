/*
 * fail.c - FAIL library functions
 *
 * Copyright (C) 2009  Piotr Fusik and Adrian Matoga
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

#include <string.h>
#include <stdio.h>

#include "fail.h"

#include "palette.h"

static const int gr15_palette_mapping[] = { 8, 4, 5, 6 };

static void decode_video_memory(const byte image[], const byte color_regs[],
								int src_start_offset, int src_stride,		/* in bytes */
								int dest_vert_offset, int dest_vert_stride,	/* in lines */
								int dest_horz_offset, int bytes_per_line,	/* in pixels, in bytes */
								int line_count, int dest_mode,
								byte frame[])
{
	int src_pos = src_start_offset;
	int dest_pos = dest_vert_offset*bytes_per_line*8;
	int x, y, i;
	byte b;
	int xe = dest_horz_offset + bytes_per_line*8;
	int xb = (dest_horz_offset > 0 ? dest_horz_offset : 0);
	if (xe > bytes_per_line*8)
		xe = bytes_per_line*8;
	
	for (y = 0; y < line_count; y++) {
		for (x = xb; x < xe; x++) {
			b = image[src_pos + (x - dest_horz_offset)/8];
			i = (x - dest_horz_offset)%8;
			switch (dest_mode) {
			case 8:
				frame[dest_pos + x] = ((b & (0x80 >> i)) == 0 ? color_regs[6] : color_regs[5]) & 0xFE;
				break;
			case 9:
				frame[dest_pos + x] = ((b & (0xF0 >> (i&4))) >> (4 - (i&4))) | (color_regs[8] & 0xF0);
				break;
			case 10:
				frame[dest_pos + x] = color_regs[(b & (0xF0 >> (i&4))) >> (4 - (i&4))];
				break;
			case 11:
				frame[dest_pos + x] = ((b & (0xF0 >> (i&4))) << (i&4)) | (color_regs[8] & 0x0F);
				frame[dest_pos + bytes_per_line*8 + x] &= 0x0F;
				frame[dest_pos + bytes_per_line*8 + x] |= frame[dest_pos + x] & 0xF0;
				break;
			case 15:
				frame[dest_pos + x] = color_regs[gr15_palette_mapping[(b & (0xC0 >> (i&6))) >> (6 - (i&6))]] & 0xFE;
			}
		}
		dest_pos += dest_vert_stride*bytes_per_line*8;
		src_pos += src_stride;
	}
}

static void frames_to_rgb(const byte frame1[], const byte frame2[],
						  const byte frame3[], const byte atari_palette[],
						  int frame_size, byte pixels[])
{
	int i;

	if (frame2 == NULL) {
		for (i = 0; i < frame_size; i++) {
			pixels[i * 3] = atari_palette[frame1[i] * 3];
			pixels[i * 3 + 1] = atari_palette[frame1[i] * 3 + 1];
			pixels[i * 3 + 2] = atari_palette[frame1[i] * 3 + 2];
		}
	}
	else {
		if (frame3 == NULL) {
			for (i = 0; i < frame_size; i++) {
				pixels[i * 3] =
					( atari_palette[frame1[i] * 3]
					+ atari_palette[frame2[i] * 3]) / 2;
				pixels[i * 3 + 1] =
					( atari_palette[frame1[i] * 3 + 1]
					+ atari_palette[frame2[i] * 3 + 1]) / 2;
				pixels[i * 3 + 2] =
					( atari_palette[frame1[i] * 3 + 2]
					+ atari_palette[frame2[i] * 3 + 2]) / 2;
			}
		}
		else {
			for (i = 0; i < frame_size; i++) {
				pixels[i * 3] =
					( atari_palette[frame1[i] * 3]
					+ atari_palette[frame2[i] * 3]
					+ atari_palette[frame3[i] * 3]) / 3;
				pixels[i * 3 + 1] =
					( atari_palette[frame1[i] * 3 + 1]
					+ atari_palette[frame2[i] * 3 + 1]
					+ atari_palette[frame3[i] * 3 + 1]) / 3;
				pixels[i * 3 + 2] =
					( atari_palette[frame1[i] * 3 + 2]
					+ atari_palette[frame2[i] * 3 + 2]
					+ atari_palette[frame3[i] * 3 + 2]) / 3;
			}
		}
	}
}

static int rgb_to_int(const byte *rgb)
{
	return rgb[0] | (rgb[1] << 8) | (rgb[2] << 16);
}

/* Count colors used and optionally convert to an image with palette.
   If palette is NULL, then only colors are counted.
   Return FALSE if image cannot be palettized. */
static abool rgb_to_palette(byte pixels[], int pixel_count, byte palette[], int *colors)
{
	int i, j;
	byte temp_palette[FAIL_PIXELS_MAX];
	int convert = 0;
	byte *pal = temp_palette;

	*colors = 0;
	for (j = 0 ; j <= convert; j++) {
		/* j == 0: count colors used, determine whether image can be palettized
		   j == 1: convert rgb pixels to palette indices */
		for (i = 0; i < pixel_count; i++) {
			abool found = FALSE;
			int cpix, cpal;
			int b = 0, e = *colors, d;
			/* binary search for rgb color value in palette */
			cpix = rgb_to_int(pixels + i*3);
			while (b < e) {
				d = b + (e - b)/2;
				cpal = rgb_to_int(pal + d*3);
				if (cpix == cpal) {
					b = d;
					found = TRUE;
					break;
				}
				if (cpix < cpal)
					e = d;
				else
					b = d + 1;
			}

			if (!found) {
				/* insert new color into palette */
				if (*colors > b)
					memmove(pal + b*3 + 3, pal + b*3, (*colors - b)*3);
				pal[b*3] = pixels[i*3];
				pal[b*3 + 1] = pixels[i*3 + 1];
				pal[b*3 + 2] = pixels[i*3 + 2];
				(*colors)++;
			}

			if (j == 1)
				pixels[i] = b;
		}
		if (palette != NULL && *colors <= 256) {
			pal = palette;
			memcpy(pal, temp_palette, FAIL_PALETTE_MAX);
			convert = 1;
		}
	}
/*	printf("\ncolors in palette: %d\n", *colors);
	int k;
	for ( k = 0; k < *colors; k++) {
		printf("%02x%02x%02x\n", palette[k*3], palette[k*3 + 1], palette[k*3 + 2]);
	}*/
	if (convert == 1)
		for (i = *colors * 3; i < FAIL_PALETTE_MAX; i++)
			palette[i] = 0;

	return TRUE;
}

static const byte gr8_color_regs[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00 };

static abool parse_binary_header(const byte *image, int *len)
{
	if (image[0] == 0xFF && image[1] == 0xFF) {
		*len =
			(image[4] | (image[5] << 8)) - 
			(image[2] | (image[3] << 8)) + 1;
		return TRUE;
	}
	else
		return FALSE;
}

static abool decode_gr8(const byte image[], int image_len,
						const byte atari_palette[],
						int *width, int *height, int *colors,
						byte pixels[], byte palette[])
{
	int offset = 0, frame_len;
	byte frame[320 * FAIL_HEIGHT_MAX];

	/* optional binary file header */
	if (parse_binary_header(image, &frame_len) == TRUE
	 && frame_len == image_len - 6)
		offset = 6;

	*width = 320;
	*height = (image_len - offset) / 40;
	if ((image_len - offset) % 40 != 0 || *height > FAIL_HEIGHT_MAX)
		return FALSE;

	decode_video_memory(image, gr8_color_regs,
						offset, 40, 0, 1, 0, 40, *height, 8,
						frame);

	frames_to_rgb(frame, NULL, NULL, atari_palette, *height * 320, pixels);

	rgb_to_palette(pixels, *height * 320, palette, colors);

	return TRUE;
}

static const byte hip_color_regs[] = { 0x00, 0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E };

static abool decode_hip(const byte image[], int image_len,
						const byte atari_palette[],
						int *width, int *height, int *colors,
						byte pixels[], byte palette[])
{
	int offset1 = 0, offset2 = 0, frame1_len, frame2_len;
	abool has_palette = FALSE;
	byte frame1[320 * FAIL_HEIGHT_MAX];
	byte frame2[320 * FAIL_HEIGHT_MAX];

	/* hip image with binary file headers */
	if (parse_binary_header(image, &frame1_len) == TRUE
	 && frame1_len > 0
	 && frame1_len * 2 + 12 == image_len
	 && frame1_len % 40 == 0
	 && parse_binary_header(image + frame1_len + 6, &frame2_len) == TRUE
	 && frame2_len == frame1_len) {
		offset1 = 6;
		offset2 = frame1_len + 12;
		*height = frame1_len / 40;
	}
	/* hip image with gr10 palette */
	else if ((image_len - 9) % 80 == 0) {
		offset1 = 0;
		offset2 = (image_len - 9) / 2;
		*height = (image_len - 9) / 80;
		has_palette = TRUE;
	}
	else
		return FALSE;

	*width = 320;
	if (*height > FAIL_HEIGHT_MAX)
		return FALSE;

	decode_video_memory(image, hip_color_regs,
						offset1, 40, 0, 1, -1, 40, *height, has_palette ? 9 : 10,
						frame1);

	decode_video_memory(image,
						has_palette ? image + image_len - 9 : hip_color_regs,
						offset2, 40, 0, 1, +1, 40, *height, has_palette ? 10 : 9,
						frame2);

	frames_to_rgb(frame1, frame2, NULL, atari_palette, *height * 320, pixels);

	rgb_to_palette(pixels, *height * 320, palette, colors);
	
	return TRUE;
}

static abool decode_mic(const byte image[], int image_len,
						const byte atari_palette[],
						int *width, int *height, int *colors,
						byte pixels[], byte palette[])
{
	abool has_palette;
	byte frame[320 * FAIL_HEIGHT_MAX];
	byte color_regs[9];
	
	memset(color_regs, 0, sizeof(color_regs));

	if (image_len % 40 == 4)
		has_palette = TRUE;
	else if (image_len % 40 == 0)
		has_palette = FALSE;
	else
		return FALSE;

	*width = 320;
	*height = (image_len - (has_palette ? 4 : 0)) / 40;
	if (*height > FAIL_HEIGHT_MAX)
		return FALSE;

	if (has_palette) {
		color_regs[4] = image[image_len - 4];
		color_regs[5] = image[image_len - 3];
		color_regs[6] = image[image_len - 2];
		color_regs[8] = image[image_len - 1];
	}
	else {
		color_regs[4] = 4;
		color_regs[5] = 8;
		color_regs[6] = 12;
		color_regs[8] = 0;
	}
		
	decode_video_memory(image, color_regs,
						0, 40, 0, 1, 0, 40, *height, 15,
						frame);

	frames_to_rgb(frame, NULL, NULL, atari_palette, *height * 320, pixels);

	rgb_to_palette(pixels, *height * 320, palette, colors);

	return TRUE;
}

static abool decode_tip(const byte image[], int image_len,
						const byte atari_palette[],
						int *width, int *height, int *colors,
						byte pixels[], byte palette[])
{
	int frame_len, line_len;
	byte frame1[320 * FAIL_HEIGHT_MAX];
	byte frame2[320 * FAIL_HEIGHT_MAX];

	if (image[0] != 'T' || image[1] != 'I' || image[2] != 'P'
	 || image[3] != 1 || image[4] != 0
	 || image[5] == 0 || image[5] > 160
	 || image[6] == 0 || image[6] > 119
	 || 9 + (frame_len = image[7] | (image[8] << 8))*3 != image_len)
		return FALSE;
	
	*width = image[5] * 2;
	*height = image[6] * 2;

	line_len = (image[5] + 3) / 4;
	decode_video_memory(image, hip_color_regs,
						9, line_len, 1, 2, -1, line_len, *height / 2, 9,
						frame1);

	decode_video_memory(image, gr8_color_regs,
						9 + 2*frame_len, line_len, 0, 2, -1, line_len, *height / 2, 11,
						frame1);

	decode_video_memory(image, hip_color_regs,
						9 + frame_len, line_len, 1, 2, +1, line_len, *height / 2, 10,
						frame2);

	decode_video_memory(image, gr8_color_regs,
						9 + 2*frame_len, line_len, 0, 2, -1, line_len, *height / 2, 11,
						frame2);

	frames_to_rgb(frame1, frame2, NULL, atari_palette, *height * *width, pixels);

	rgb_to_palette(pixels, *height * *width, palette, colors);
	
	return TRUE;
}

#define FAIL_EXT(c1, c2, c3) (((c1) + ((c2) << 8) + ((c3) << 16)) | 0x202020)

static int get_packed_ext(const char *filename)
{
	const char *p;
	int ext;
	for (p = filename; *p != '\0'; p++);
	ext = 0;
	for (;;) {
		if (--p <= filename || *p <= ' ')
			return 0; /* no filename extension or invalid character */
		if (*p == '.')
			return ext | 0x202020;
		ext = (ext << 8) + (*p & 0xff);
	}
}

static abool is_our_ext(int ext)
{
	switch (ext) {
	case FAIL_EXT('G', 'R', '8'):
	case FAIL_EXT('H', 'I', 'P'):
	case FAIL_EXT('M', 'I', 'C'):
	case FAIL_EXT('T', 'I', 'P'):
		return TRUE;
	default:
		return FALSE;
	}
}

abool FAIL_IsOurFile(const char *filename)
{
	int ext = get_packed_ext(filename);
	return is_our_ext(ext);
}

abool FAIL_DecodeImage(const char *filename,
                       const byte image[], int image_len,
                       const byte atari_palette[],
                       int *width, int *height, int *colors,
                       byte pixels[], byte palette[])
{
	if (atari_palette == NULL)
		atari_palette = jakub_act;

	int ext = get_packed_ext(filename);

	switch (ext) {
	case FAIL_EXT('G', 'R', '8'):
		return decode_gr8(image, image_len, atari_palette,
						  width, height, colors,
						  pixels, palette);
	case FAIL_EXT('H', 'I', 'P'):
		return decode_hip(image, image_len, atari_palette,
						  width, height, colors,
						  pixels, palette);
	case FAIL_EXT('M', 'I', 'C'):
		return decode_mic(image, image_len, atari_palette,
						  width, height, colors,
						  pixels, palette);
	case FAIL_EXT('T', 'I', 'P'):
		return decode_tip(image, image_len, atari_palette,
						  width, height, colors,
						  pixels, palette);
	default:
		return FALSE;
	}
}
