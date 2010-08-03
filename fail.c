/*
 * fail.c - FAIL library functions
 *
 * Copyright (C) 2009-2010  Piotr Fusik and Adrian Matoga
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

#define FAIL_MODE_CIN15 31
#define FAIL_MODE_MULTIRIP 48

static void decode_video_memory(
	const byte image[], const byte color_regs[],
	int src_start_offset, int src_stride,	/* in bytes */
	int dest_vert_offset, int dest_vert_stride,	/* in lines */
	int dest_horz_offset, int bytes_per_line,	/* in pixels, in bytes */
	int line_count, int dest_mode, byte frame[])
{
	static const byte gr10_to_reg[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 8, 8, 8, 4, 5, 6, 7 };
	int pixels_per_line = bytes_per_line * 8;
	int src_pos = src_start_offset;
	int dest_pos = dest_vert_offset * pixels_per_line;
	int odd = dest_vert_offset & 1;
	int pa = odd ? -pixels_per_line : pixels_per_line;
	int x;
	int y;
	int i;
	int col;
	byte b;
	int xe = dest_horz_offset + bytes_per_line * 8;
	int xb = (dest_horz_offset > 0 ? dest_horz_offset : 0);
	if (xe > bytes_per_line * 8)
		xe = bytes_per_line * 8;
	for (y = 0; y < line_count; y++) {
		for (x = 0; x < xb; x++)
			frame[dest_pos + x] = 0;
		for (x = xb; x < xe; x++) {
			b = image[src_pos + (x - dest_horz_offset) / 8];
			i = (x - dest_horz_offset) % 8;
			switch (dest_mode) {
			case 8:
				/* color_regs should be 2 bytes long */
				frame[dest_pos + x] = color_regs[b >> (7 - i) & 1] & 0xFE;
				break;
			case 9:
				/* 1 reg, only hue is meaningful */
				frame[dest_pos + x] = (b >> (~i & 4) & 0x0F) | (color_regs[0] & 0xFE);
				break;
			case 10:
				/* 16 regs, typically only colors 0-8 are used */
				frame[dest_pos + x] = color_regs[gr10_to_reg[b >> (~i & 4) & 0x0F]] & 0xFE;
				break;
			case 11:
				/* copy hue to the other line so lines 2k and 2k+1 have the same
				   hue (emulating PAL color resolution reduction), and average intensity
				   from both neighboring lines (to avoid distortion in tip/apac modes
				   with non-integer zoom factors) */
				{
					int hu = b << (i & 4) & 0xF0;
					int in =
						((y == 0 && !odd ? 0 : frame[dest_pos - pixels_per_line + x] & 0x0F) +
						(y == line_count - 1 && odd ? 0 : frame[dest_pos + pixels_per_line + x] & 0x0F)) / 2;
					frame[dest_pos + x] = hu | in;
					frame[dest_pos + pa + x] = (frame[dest_pos + pa + x] & 0x0F) | hu;
				}
				break;
			case 15: /* 4 regs */
				frame[dest_pos + x] = color_regs[b >> (~i & 6) & 3] & 0xFE;
				break;
			case FAIL_MODE_CIN15: /* 4 * 256 regs, only first 192 of each 256-byte group are used */
				frame[dest_pos + x] = color_regs[(b >> (~i & 6) & 3) * 256 + y] & 0xFE;
				break;
			case FAIL_MODE_MULTIRIP:
				col = b >> (~i & 4) & 0x0F;
				frame[dest_pos + x] = col == 0 ? 0 : color_regs[col + (y / 2) * 8 - 1] & 0xFE;
				break;
			}
		}
		for ( /* x = xe */ ; x < bytes_per_line * 8; x++)
			frame[dest_pos + x] = 0;
		dest_pos += dest_vert_stride * bytes_per_line * 8;
		src_pos += src_stride;
	}
}

static void frame_to_rgb(
	const byte frame[], int frame_size,
	const byte atari_palette[],
	byte pixels[])
{
	int i;
	for (i = 0; i < frame_size; i++) {
		pixels[i * 3] = atari_palette[frame[i] * 3];
		pixels[i * 3 + 1] = atari_palette[frame[i] * 3 + 1];
		pixels[i * 3 + 2] = atari_palette[frame[i] * 3 + 2];
	}
}

/* mix interlaced frames */
static void frames_to_rgb(
	const byte frame1[], const byte frame2[],
	int frame_size,
	const byte atari_palette[],
	byte pixels[])
{
	int i;
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

/* mix 3 interlaced frames (for "RGB" atari images) */
/*static void frames_to_rgb_3(
	const byte frame1[], const byte frame2[],
	const byte frame3[],
	const byte atari_palette[],
	int frame_size, byte pixels[])
{
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
}*/

static int rgb_to_int(const byte *rgb)
{
	return rgb[0] | (rgb[1] << 8) | (rgb[2] << 16);
}

/* Binary search for RGB color value in palette.
   index is set to the position of first color in palette
   greater or equal (in order defined by rgb_to_int)
   to given rgb value. */
static abool find_rgb_color(
	const byte palette[], int colors,
	int rgb, int *index)
{
	int b = 0;
	int e = colors;
	int d;
	while (b < e) {
		int cpal;
		d = b + (e - b) / 2;
		cpal = rgb_to_int(palette + d * 3);
		if (rgb == cpal) {
			*index = d;
			return TRUE;
		}
		if (rgb < cpal)
			e = d;
		else
			b = d + 1;
	}
	*index = b;
	return FALSE;
}

/* Count colors used and optionally convert to an image with palette.
   If palette is NULL, then only colors are counted.
   Return FALSE if image cannot be palettized. */
static abool rgb_to_palette(
	byte pixels[], int pixel_count,
	byte palette[], int *colors)
{
	int i;
	byte temp_palette[FAIL_PIXELS_MAX];

	/* count colors used, determine whether image can be palettized */
	*colors = 0;
	for (i = 0; i < pixel_count; i++) {
		int index;
		if (!find_rgb_color(temp_palette, *colors, rgb_to_int(pixels + i * 3), &index)) {
			/* insert new color into palette */
			if (*colors > index)
				memmove(temp_palette + index * 3 + 3,
					temp_palette + index * 3, (*colors - index) * 3);
			temp_palette[index * 3] = pixels[i * 3];
			temp_palette[index * 3 + 1] = pixels[i * 3 + 1];
			temp_palette[index * 3 + 2] = pixels[i * 3 + 2];
			(*colors)++;
		}
	}

	/* convert rgb pixels to palette indices */
	if (palette == NULL || *colors > 256)
		return FALSE;
	
	memcpy(palette, temp_palette, FAIL_PALETTE_MAX);
	for (i = 0; i < pixel_count; i++) {
		int index;
		if (find_rgb_color(temp_palette, *colors, rgb_to_int(pixels + i * 3), &index))
			pixels[i] = index;
	}
	/* pad palette with 0s */
	for (i = *colors * 3; i < FAIL_PALETTE_MAX; i++)
		palette[i] = 0;

	return TRUE;
}

static abool unpack_koala(const byte data[], int data_len, int cprtype, byte unpacked_data[])
{
	int i;
	int d;
	switch (cprtype) {
	case 0:
		if (data_len != 7680)
			return FALSE;
		memcpy(unpacked_data, data, 7680);
		return TRUE;
	case 1:
	case 2:
		break;
	default:
		return FALSE;
	}
	i = 0;
	d = 0;
	for (;;) {
		int c;
		int len;
		int b;
		c = data[d++];
		if (d > data_len)
			return FALSE;
		len = c & 0x7f;
		if (len == 0) {
			int h;
			h = data[d++];
			if (d > data_len)
				return FALSE;
			len = data[d++];
			if (d > data_len)
				return FALSE;
			len += h << 8;
			if (len == 0)
				return FALSE;
		}
		b = -1;
		do {
			/* get byte if uncompressed block
			   or if starting RLE block */
			if (c >= 0x80 || b < 0) {
				b = data[d++];
				if (d > data_len)
					return FALSE;
			}
			unpacked_data[i] = (byte) b;
			/* return if last byte written */
			if (i >= 7679)
				return TRUE;
			if (cprtype == 2)
				i++;
			else {
				i += 80;
				if (i >= 7680) {
					/* if in line 192, back to odd lines in the same column;
					   if in line 193, go to even lines in the next column */
					i -= (i < 7720) ? 191 * 40 : 193 * 40 - 1;
				}
			}
		} while (--len > 0);
	}
}

static abool unpack_cci(const byte data[], int data_len, int step, int count, byte unpacked_data[])
{
	int i = 0;
	int d = 2;
	int size = step * count;
	int block_count = data[0] + (data[1] << 8);
	while (block_count) {
		int c;
		int len;
		int b;
		if (d > data_len)
			return FALSE;
		c = data[d++];
		len = (c & 0x7f) + 1;
		b = -1;
		do {
			/* get byte if uncompressed block
			   or if starting RLE block */
			if (c < 0x80 || b < 0) {
				if (d > data_len)
					return FALSE;
				b = data[d++];
			}
			unpacked_data[i] = (byte) b;
			/* return if last byte written */
			if (i >= size - 1)
				return TRUE;
			i += step;
			if (i >= size)
				i -= size - 1;
		} while (--len > 0);
		block_count--;
	}
	if (d == data_len)
		return TRUE;
	else
		return FALSE;
}

/* unpack_rip* are directly translated from Visage 2.7 assembly source.
   TODO: may need to be rewritten in more understandable way */

static void unpack_rip_cnibl(const byte data[], int size, byte output[])
{
	int x = 0;
	int y = 0;
	while (y < size) {
		byte a = data[x++];
		output[y++] = a >> 4;
		output[y++] = a & 0x0f;
	}
}

static void unpack_rip_sort(const byte data[], int size, byte tre01[], byte tre02[])
{
	byte pom[16];
	int y;
	int x;
	int md_ = 0;
	int md = 0;

	unpack_rip_cnibl(data, size, tre02);
	memset(pom, 0, sizeof(pom));
	for (y = 0; y < size; y++)
		pom[tre02[y]]++;

	x = 0;
	do {
		y = 0;
		do {
			if (x == tre02[y])
				tre01[md_++] = y;
			y++;
		} while (y < size);
		x++;
	} while (x < 16);
	
	x = 0;
	do {
		y = pom[x];
		while (y) {
			tre02[md++] = x;
			y--;
		}
		x++;
	} while (x < 16);
}

static void unpack_rip_fano(const byte data[], int size, byte tre01[], byte tre02[], byte l0[], byte h0[], byte l1[], byte h1[])
{
	int p;
	int err;
	int l;
	int nxt;
	int y;

	unpack_rip_sort(data, size, tre01, tre02);

	memset(l0, 0, size);
	memset(l1, 0, size);
	memset(h0, 0, size);
	memset(h1, 0, size);

	p = 0;
	err = 0;
	l = 0;
	nxt = 0;
	y = 0;
	do {
		if (tre02[y]) {
			int x;
			int tmp;
			int val;
			int a;
			p += err;
			x = tre02[y];
			if (x != l) {
				l = x;
				err = 0x10000 >> x;
			}
			tmp = p;
			val = tre01[y];
			x = tre02[y];
			a = 0;
			for (;;) {
				int z = a;
				x--;
				tmp <<= 1;
				if (tmp < 0x10000) {
					if (x == 0) {
						a = val;
						l0[z] = a;
						break;
					}
					a = h0[z];
					if (a == 0) {
						a = ++nxt;
						h0[z] = a;
					}
				}
				else {
					tmp &= 0xFFFF;
					if (x == 0) {
						a = val;
						l1[z] = a;
						break;
					}
					a = h1[z];
					if (a == 0) {
						a = ++nxt;
						h1[z] = a;
					}
				}
			}
		}
		y++;
	} while (y < size);
}

static abool unpack_rip(const byte data[], int data_len, byte unpacked_data[])
{
	byte adl0[576];
	byte adh0[576];
	byte adl1[576];
	byte adh1[576];
	
	byte tre01[256];
	byte tre02[256];

	int unpacked_len;
	int sx;
	int dx;
	int cx;
	byte csh = 0;
	byte c;
	int lic;

	/* "PCK" header (16 bytes) + 288 bytes shannon-fano */
	if (data_len < 304 || data[0] != 'P' || data[1] != 'C' || data[2] != 'K')
		return FALSE;

	unpacked_len = data[4] + 256 * data[5] - 33;
	if (unpacked_len > 0x5EFE)
		return FALSE;

	unpack_rip_fano(data + 16, 64, tre01, tre02, adl0, adh0, adl1, adh1);
	unpack_rip_fano(data + 16 + 32, 256, tre01, tre02, adl0 + 64, adh0 + 64, adl1 + 64, adh1 + 64);
	unpack_rip_fano(data + 16 + 160, 256, tre01, tre02, adl0 + 320, adh0 + 320, adl1 + 320, adh1 + 320);

	sx = 16 + 288;
	dx = 0;
	lic = -1;

#define GBIT \
	do { \
		if (--lic < 0) { \
			if (sx >= data_len) \
				return FALSE; \
			csh = data[sx++]; \
			lic = 7; \
		} \
		c = csh & (1 << lic); \
	} while (FALSE)

	do {
		GBIT;
		if (!c) {
			int a = 0;
			for (;;) {
				int y = a;
				GBIT;
				if (!c) {
					if ((a = adh0[320 + y]) == 0) {
						unpacked_data[dx] = adl0[320 + y];
						break;
					}
				}
				else {
					if ((a = adh1[320 + y]) == 0) {
						unpacked_data[dx] = adl1[320 + y];
						break;
					}
				}
			}
			++dx;
		}
 		else {
			int a = 0;
			for (;;) {
				int y = a;
				GBIT;
				if (!c) {
					if ((a = adh0[64 + y]) == 0) {
						a = adl0[64 + y];
						break;
					}
				}
				else {
					if ((a = adh1[64 + y]) == 0) {
						a = adl1[64 + y];
						break;
					}
				}
			}
			cx = dx - (a + 2);
			a = 0;
			for (;;) {
				int y = a;
				GBIT;
				if (!c) {
					if ((a = adh0[y]) == 0) {
						a = adl0[y];
						break;
					}
				}
				else {
					if ((a = adh1[y]) == 0) {
						a = adl1[y];
						break;
					}
				}
			}
			memcpy(unpacked_data + dx, unpacked_data + cx, a + 2);
			dx += a + 2;
		}
	} while (dx < unpacked_len);
 
	return TRUE;
}

static abool parse_binary_header(const byte image[], int *len)
{
	if (image[0] == 0xFF && image[1] == 0xFF) {
		int start_address = image[4] | (image[5] << 8);
		int end_address = image[2] | (image[3] << 8);
		*len = start_address - end_address + 1;
		return TRUE;
	}
	else
		return FALSE;
}

static const byte gr8_color_regs[] = { 0x00, 0x0F };

static abool decode_gr8_gr9(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[],
	int dest_mode, const byte color_regs[])
{
	int offset = 0;
	int frame_len;
	byte frame[320 * FAIL_HEIGHT_MAX];

	/* optional binary file header */
	if (parse_binary_header(image, &frame_len) 
	 && frame_len == image_len - 6)
		offset = 6;

	image_info->width = 320;
	image_info->height = (image_len - offset) / 40;
	image_info->original_height = image_info->height;
		
	if ((image_len - offset) % 40 != 0 || image_info->height > FAIL_HEIGHT_MAX)
		return FALSE;
	
	decode_video_memory(
		image, color_regs,
		offset, 40, 0, 1, 0, 40, image_info->height, dest_mode,
		frame);

	frame_to_rgb(frame, image_info->height * 320, atari_palette, pixels);

	return TRUE;
}

static abool decode_gr8(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	image_info->original_width = 320;
	return decode_gr8_gr9(
		image, image_len, atari_palette,
		image_info,
		pixels, 8, gr8_color_regs);
}

static abool decode_gr9(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	image_info->original_width = 80;
	return decode_gr8_gr9(
		image, image_len, atari_palette,
		image_info,
		pixels, 9, gr8_color_regs);
}

static abool decode_hr(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	byte frame1[256 * 239];
	byte frame2[256 * 239];

	if (image_len != 16384)
		return FALSE;

	image_info->width = 256;
	image_info->height = 239;
	image_info->original_width = 256;
	image_info->original_height = 239;
	
	decode_video_memory(
		image, gr8_color_regs,
		0, 32, 0, 1, 0, 32, 239, 8,
		frame1);

	decode_video_memory(
		image + 8192, gr8_color_regs,
		0, 32, 0, 1, 0, 32, 239, 8,
		frame2);

	frames_to_rgb(frame1, frame2, 256 * 239, atari_palette, pixels);

	return TRUE;
}

static const byte hip_color_regs[] = { 0x00, 0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E };

static abool decode_hip(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	int offset1 = 0;
	int offset2 = 0;
	int frame1_len;
	int frame2_len;
	abool has_palette = FALSE;
	byte frame1[320 * FAIL_HEIGHT_MAX];
	byte frame2[320 * FAIL_HEIGHT_MAX];

	/* hip image with binary file headers */
	if (parse_binary_header(image, &frame1_len)
	 && frame1_len > 0
	 && frame1_len * 2 + 12 == image_len
	 && frame1_len % 40 == 0
	 && parse_binary_header(image + frame1_len + 6, &frame2_len)
	 && frame2_len == frame1_len) {
		offset1 = 6;
		offset2 = frame1_len + 12;
		image_info->height = frame1_len / 40;
	}
	/* hip image with gr10 palette */
	else if ((image_len - 9) % 80 == 0) {
		offset1 = 0;
		offset2 = (image_len - 9) / 2;
		image_info->height = (image_len - 9) / 80;
		has_palette = TRUE;
	}
	else
		return FALSE;

	image_info->width = 320;
	if (image_info->height > FAIL_HEIGHT_MAX)
		return FALSE;
	image_info->original_width = 160;
	image_info->original_height = image_info->height;

	decode_video_memory(
		image, hip_color_regs,
		offset1, 40, 0, 1, -1, 40, image_info->height, has_palette ? 9 : 10,
		frame1);

	decode_video_memory(
		image, has_palette ? image + image_len - 9 : hip_color_regs,
		offset2, 40, 0, 1, +1, 40, image_info->height, has_palette ? 10 : 9,
		frame2);

	frames_to_rgb(frame1, frame2, image_info->height * 320, atari_palette, pixels);

	return TRUE;
}

static const byte mic_color_regs[] = { 0, 4, 8, 12 };

static abool decode_mic(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	abool has_palette;
	byte frame[320 * 192];
	byte color_regs[9];
	
	memset(color_regs, 0, sizeof(color_regs));

	if (image_len % 40 == 4)
		has_palette = TRUE;
	else if (image_len % 40 == 0)
		has_palette = FALSE;
	else
		return FALSE;

	image_info->width = 320;
	image_info->height = image_len / 40;
	if (image_info->height > FAIL_HEIGHT_MAX)
		return FALSE;
	image_info->original_width = 320;
	image_info->original_height = image_info->height;

	decode_video_memory(
		image, has_palette ? image + image_len - 4 : mic_color_regs,
		0, 40, 0, 1, 0, 40, image_info->height, 15,
		frame);

	frame_to_rgb(frame, image_info->height * 320, atari_palette, pixels);

	return TRUE;
}

static abool decode_pic(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	/* some images with .pic extension are
	   in micropainter format */
	if (image_len == 7684 || image_len == 7680)
		return decode_mic(
			image, image_len, atari_palette,
			image_info, pixels);

	else {
		byte unpacked_image[7680 + 4];

		if (image[0] != 0xff || image[1] != 0x80
		 || image[2] != 0xc9 || image[3] != 0xc7
		 || image[4] < 0x1a || image[4] >= image_len
		 || image[5] != 0
		 || image[6] != 1 || image[8] != 0x0e
		 || image[9] != 0 || image[10] != 40
		 || image[11] != 0 || image[12] != 192
		 || image[20] != 0 || image[21] != 0)
			return FALSE;
		
		if (!unpack_koala(
			image + image[4] + 1, image_len - image[4] - 1,
			image[7], unpacked_image))
			return FALSE;

		unpacked_image[7680] = image[17];
		unpacked_image[7681] = image[13];
		unpacked_image[7682] = image[14];
		unpacked_image[7683] = image[15];
		
		return decode_mic(
			unpacked_image, 7684, atari_palette,
			image_info, pixels);
	}
}

static abool decode_cpr(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	byte unpacked_image[7680];

	if (!unpack_koala(
		image + 1, image_len - 1,
		image[0], unpacked_image))
		return FALSE;

	return decode_gr8(
		unpacked_image, 7680, atari_palette,
		image_info, pixels);
}

static abool decode_int(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	byte frame1[320 * FAIL_HEIGHT_MAX];
	byte frame2[320 * FAIL_HEIGHT_MAX];

	if (image[0] != 'I' || image[1] != 'N' || image[2] != 'T'
	 || image[3] != '9' || image[4] != '5' || image[5] != 'a'
	 || image[6] == 0 || image[6] > 40
	 || image[7] == 0 || image[7] > 239
	 || image[8] != 0x0f || image[9] != 0x2b
	 || 18 + image[6] * image[7] * 2 != image_len)
		return FALSE;

	image_info->width = image[6] * 8;
	image_info->height = image[7];
	image_info->original_width = image[6] * 4;
	image_info->original_height = image[7];

	decode_video_memory(
		image + 18, image + 10,
		0, 40, 0, 1, 0, 40, image_info->height, 15,
		frame1);

	decode_video_memory(
		image + 18 + image[6] * image_info->height, image + 14,
		0, 40, 0, 1, 0, 40, image_info->height, 15,
		frame2);

	frames_to_rgb(frame1, frame2, image_info->height * 320, atari_palette, pixels);

	return TRUE;
}

static abool decode_inp(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	byte frame1[320 * 200];
	byte frame2[320 * 200];

	if (image_len < 16004)
		return FALSE;

	image_info->width = 320;
	image_info->height = 200;
	image_info->original_width = 160;
	image_info->original_height = 200;

	decode_video_memory(
		image, image + 16000,
		0, 40, 0, 1, 0, 40, image_info->height, 15,
		frame1);

	decode_video_memory(
		image + 8000, image + 16000,
		0, 40, 0, 1, 0, 40, image_info->height, 15,
		frame2);

	frames_to_rgb(frame1, frame2, image_info->height * 320, atari_palette, pixels);

	return TRUE;
}

static abool decode_cin(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	byte frame1[320 * 192];
	byte frame2[320 * 192];

	if (image_len != 16384)
		return FALSE;
	
	image_info->width = 320;
	image_info->height = 192;
	image_info->original_width = 160;
	image_info->original_height = 192;

	decode_video_memory(
		image, image + 0x3C00,
		40, 80, 1, 2, 0, 40, 96, FAIL_MODE_CIN15,
		frame1);

	decode_video_memory(
		image, gr8_color_regs,
		7680, 80, 0, 2, 0, 40, 96, 11,
		frame1);

	decode_video_memory(
		image, image + 0x3C00,
		0, 80, 0, 2, 0, 40, 96, FAIL_MODE_CIN15,
		frame2);

	decode_video_memory(
		image, gr8_color_regs,
		7720, 80, 1, 2, 0, 40, 96, 11,
		frame2);

	frames_to_rgb(frame1, frame2, 320 * 192, atari_palette, pixels);

	return TRUE;
}

static abool decode_cci(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	const byte *data;
	int data_len;
	byte unpacked_image[16384];
	memset(unpacked_image, 0, sizeof(unpacked_image));

	if (image[0] != 'C' || image[1] != 'I' || image[2] != 'N' || image[3] != ' '
	 || image[4] != '1' || image[5] != '.' || image[6] != '2' || image[7] != ' ')
		return FALSE;

	/* compressed even lines of gr15 frame */
	data = image + 8;
	data_len = data[0] + (data[1] << 8);
	if (!unpack_cci(data + 2, data_len, 80, 96, unpacked_image))
		return FALSE;
	
	/* compressed odd lines of gr15 frame */
	data += 2 + data_len;
	data_len = data[0] + (data[1] << 8);
	if (!unpack_cci(data + 2, data_len, 80, 96, unpacked_image + 40))
		return FALSE;

	/* compressed gr11 frame */
	data += 2 + data_len;
	data_len = data[0] + (data[1] << 8);
	if (!unpack_cci(data + 2, data_len, 40, 192, unpacked_image + 7680))
		return FALSE;

	/* compressed color values for gr15 */
	data += 2 + data_len;
	data_len = data[0] + (data[1] << 8);
	if (!unpack_cci(data + 2, data_len, 1, 0x400, unpacked_image + 0x3C00))
		return FALSE;

	if (data + 2 + data_len != image + image_len)
		return FALSE;

	return decode_cin(
		unpacked_image, 16384, atari_palette,
		image_info, pixels);
}

static abool decode_tip(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	int frame_len;
	int line_len;
	byte frame1[320 * FAIL_HEIGHT_MAX];
	byte frame2[320 * FAIL_HEIGHT_MAX];

	if (image[0] != 'T' || image[1] != 'I' || image[2] != 'P'
	 || image[3] != 1 || image[4] != 0
	 || image[5] == 0 || image[5] > 160
	 || image[6] == 0 || image[6] > 119
	 || 9 + (frame_len = image[7] | (image[8] << 8)) * 3 != image_len)
		return FALSE;
	
	image_info->width = image[5] * 2;
	image_info->height = image[6] * 2;
	image_info->original_width = image[5];
	image_info->original_height = image[6];

	line_len = (image_info->original_width + 3) / 4;
	/* even frame, gr11 + gr9 */
	decode_video_memory(
		image, hip_color_regs, 
		9, line_len, 1, 2, -1, line_len, image[6], 9,
		frame1);

	decode_video_memory(
		image, gr8_color_regs,
		9 + 2 * frame_len, line_len, 0, 2, -1, line_len, image[6], 11,
		frame1);

	/* odd frame, gr11 + gr10 */
	decode_video_memory(
		image, hip_color_regs,
		9 + frame_len, line_len, 1, 2, +1, line_len, image[6], 10,
		frame2);

	decode_video_memory(
		image, gr8_color_regs,
		9 + 2 * frame_len, line_len, 0, 2, -1, line_len, image[6], 11,
		frame2);

	frames_to_rgb(frame1, frame2,
		image_info->height * image_info->width, atari_palette, pixels);

	return TRUE;
}

/* serves both APC and PLM formats */
static abool decode_apc(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	byte frame[320 * 192];
	
	if (image_len != 7680 && image_len != 7720)
		return FALSE;
	
	image_info->width = 320;
	image_info->height = 192;
	image_info->original_width = 80;
	image_info->original_height = 96;

	decode_video_memory(
		image, hip_color_regs, 
		40, 80, 1, 2, 0, 40, 96, 9,
		frame);

	decode_video_memory(
		image, gr8_color_regs,
		0, 80, 0, 2, 0, 40, 96, 11,
		frame);

	frame_to_rgb(frame, image_info->height * image_info->width,
		atari_palette, pixels);

	return TRUE;
}

/* serves both AP3 and ILC */
static abool decode_ap3(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	byte frame1[320 * 192];
	byte frame2[320 * 192];

	if (image_len != 15872 && image_len != 15360)
		return FALSE;

	image_info->width = 320;
	image_info->height = 192;
	image_info->original_width = 80;
	image_info->original_height = 192;

	decode_video_memory(
		image, hip_color_regs, 
		40, 40, 1, 2, 0, 40, 96, 9,
		frame1);

	decode_video_memory(
		image, gr8_color_regs,
		image_len == 15360 ? 7680 : 8192, 40, 0, 2, 0, 40, 96, 11,
		frame1);

	decode_video_memory(
		image, hip_color_regs,
		0, 40, 0, 2, 0, 40, 96, 9,
		frame2);

	decode_video_memory(
		image, gr8_color_regs,
		image_len == 15360 ? 7720 : 8232, 40, 1, 2, 0, 40, 96, 11,
		frame2);

	frames_to_rgb(frame1, frame2,
		image_info->height * image_info->width, atari_palette, pixels);

	return TRUE;
}

static abool decode_rip(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	byte unpacked_image[24576];
	byte frame1[FAIL_WIDTH_MAX * FAIL_HEIGHT_MAX];
	byte frame2[FAIL_WIDTH_MAX * FAIL_HEIGHT_MAX];
	int txt_len = image[17];
	int pal_len = image[20 + txt_len];
	int hdr_len = image[11] + 256 * image[12];
	int data_len = image_len - hdr_len;
	int line_len;
	int frame_len;

	if (image[0] != 'R' || image[1] != 'I' || image[2] != 'P'
	 || image[13] > 80 || image[15] > 239 || txt_len > 152
	 || image[18] != 'T' || image[19] != ':' || pal_len != 9
	 || image[21 + txt_len] != 'C' || image[22 + txt_len] != 'M'
	 || image[23 + txt_len] != ':')
		return FALSE;

	switch (image[9]) {
	case 0:
		if (data_len > 24576)
			return FALSE;
		memcpy(unpacked_image, image + hdr_len, data_len);
		break;
	case 1:
		if (unpack_rip(image + hdr_len, data_len, unpacked_image))
			break;
	default:
		return FALSE;
	}

	image_info->width = image[13] * 4;
	image_info->height = image[15];
	image_info->original_width = image[13] * 2;
	image_info->original_height = image[15];

	line_len = image[13] / 2;
	frame_len = line_len * image_info->height;

	switch (image[7]) {
	case 0x20:
		/* hip, rip */
		decode_video_memory(
			unpacked_image, image + 24 + txt_len,
			0, line_len, 0, 1, -1, line_len, image_info->height,
			10, frame1);
		decode_video_memory(
			unpacked_image, hip_color_regs,
			frame_len, line_len, 0, 1, +1, line_len, image_info->height,
			9, frame2);
		break;
	case 0x30:
		/* multi rip */
		{
			int x, y;
			for (y = 0; y < 119; y++) {
				for (x = 0; x < 8; x++) {
					int ix = 2 * frame_len + y * 8 + x;
					if (y > 0 && unpacked_image[ix] == 0)
						unpacked_image[ix] = unpacked_image[ix - 8];
				}
			}
		}
		decode_video_memory(
			unpacked_image, unpacked_image + 2 * frame_len,
			0, line_len, 0, 1, -1, line_len, image_info->height,
			FAIL_MODE_MULTIRIP, frame1);
		decode_video_memory(
			unpacked_image, hip_color_regs,
			frame_len, line_len, 0, 1, +1, line_len, image_info->height,
			9, frame2);
		break;
	default:
		return FALSE;
	}

	frames_to_rgb(frame1, frame2,
		image_info->height * image_info->width, atari_palette, pixels);

	return TRUE;
}

static abool decode_fonts(
	const byte ordered_bytes[],
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	byte frame[256 * 32];

	image_info->width = 256;
	image_info->height = 32;
	image_info->original_width = 256;
	image_info->original_height = 32;

	decode_video_memory(
		ordered_bytes, gr8_color_regs,
		0, 32, 0, 1, 0, 32, 32, 8, frame);
	
	frame_to_rgb(frame, 256 * 32, atari_palette, pixels);

	return TRUE;
}

static abool decode_fnt(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	byte ordered_bytes[1024];
	int i;
	int len;
	const byte *s;

	if (image_len == 1024)
		s = image;
	else if (image_len == 1030 && parse_binary_header(image, &len) && len == 1024)
		s = image + 6;
	else
		return FALSE;

	for (i = 0; i < 1024; i++) {
		ordered_bytes[
			(i & 0x300)
			+ ((i & 7) << 5)
			+ ((i >> 3) & 0x1f)
		] = s[i];	
	}

	return decode_fonts(ordered_bytes, atari_palette,
		image_info, pixels);
}

static abool decode_sxs(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	byte ordered_bytes[1024];
	int i;
	int len;

	if (image_len != 1030 || !parse_binary_header(image, &len) || len != 1024)
		return FALSE;
	
	for (i = 0; i < 1024; i++) {
		ordered_bytes[
			(i & 0x200)
			+ ((i & 16) << 4)
			+ ((i & 7) << 5)
			+ ((i >> 4) & 0x1e)
			+ ((i >> 3) & 1)
		] = image[6 + i];
	}

	return decode_fonts(ordered_bytes, atari_palette,
		image_info, pixels);
}

static abool decode_mcp(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	byte frame1[320 * 200];
	byte frame2[320 * 200];

	if (image_len != 16008)
		return FALSE;

	{
		byte colors1[4] = { image[16003], image[16000], image[16001], image[16002] };
		byte colors2[4] = { image[16007], image[16004], image[16005], image[16006] };

		image_info->width = 320;
		image_info->height = 200;
		image_info->original_width = 160;
		image_info->original_height = 200;

		decode_video_memory(
			image, colors1,
			0, 80, 0, 2, 0, 40, 100, 15,
			frame1);

		decode_video_memory(
			image, colors2,
			40, 80, 1, 2, 0, 40, 100, 15,
			frame1);

		decode_video_memory(
			image, colors2,
			8000, 80, 0, 2, 0, 40, 100, 15,
			frame2);

		decode_video_memory(
			image, colors1,
			8040, 80, 1, 2, 0, 40, 100, 15,
			frame2);
	}

	frames_to_rgb(frame1, frame2, 320 * 200, atari_palette, pixels);

	return TRUE;
}

static abool decode_ghg(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	static const byte ghg_color_regs[] = { 0x0C, 0x02 };
	byte frame[320 * 200];
	if (image_len < 4)
		return FALSE;
	// round up to 8 pixels
	image_info->original_width = image_info->width = (image[0] + image[1] * 256 + 7) & ~7;
	image_info->original_height = image_info->height = image[2];
	if (image_info->width == 0 || image_info->width > 320
	 || image_info->height == 0 || image_info->height > 200)
		return FALSE;

	decode_video_memory(
		image, ghg_color_regs,
		3, image_info->width >> 3, 0, 1, 0, image_info->width >> 3, image_info->height, 8,
		frame);

	frame_to_rgb(frame, image_info->width * image_info->height, atari_palette, pixels);

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
	case FAIL_EXT('I', 'N', 'T'):
	case FAIL_EXT('I', 'N', 'P'):
	case FAIL_EXT('H', 'R', ' '):
	case FAIL_EXT('G', 'R', '9'):
	case FAIL_EXT('P', 'I', 'C'):
	case FAIL_EXT('C', 'P', 'R'):
	case FAIL_EXT('C', 'I', 'N'):
	case FAIL_EXT('C', 'C', 'I'):
	case FAIL_EXT('A', 'P', 'C'):
	case FAIL_EXT('P', 'L', 'M'):
	case FAIL_EXT('A', 'P', '3'):
	case FAIL_EXT('I', 'L', 'C'):
	case FAIL_EXT('R', 'I', 'P'):
	case FAIL_EXT('F', 'N', 'T'):
	case FAIL_EXT('S', 'X', 'S'):
	case FAIL_EXT('M', 'C', 'P'):
	case FAIL_EXT('G', 'H', 'G'):
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
	FAIL_ImageInfo* image_info,
	byte pixels[], byte palette[])
{
	int ext;
	static const struct {
		int ext;
		int (*func)(
			const byte[], int, const byte[],
			FAIL_ImageInfo*, byte[]);
	} handlers[] = {
		{ FAIL_EXT('G', 'R', '8'), decode_gr8 },
		{ FAIL_EXT('H', 'I', 'P'), decode_hip },
		{ FAIL_EXT('M', 'I', 'C'), decode_mic },
		{ FAIL_EXT('I', 'N', 'T'), decode_int },
		{ FAIL_EXT('T', 'I', 'P'), decode_tip },
		{ FAIL_EXT('I', 'N', 'P'), decode_inp },
		{ FAIL_EXT('H', 'R', ' '), decode_hr },
		{ FAIL_EXT('G', 'R', '9'), decode_gr9 },
		{ FAIL_EXT('P', 'I', 'C'), decode_pic },
		{ FAIL_EXT('C', 'P', 'R'), decode_cpr },
		{ FAIL_EXT('C', 'I', 'N'), decode_cin },
		{ FAIL_EXT('C', 'C', 'I'), decode_cci },
		{ FAIL_EXT('A', 'P', 'C'), decode_apc },
		{ FAIL_EXT('P', 'L', 'M'), decode_apc },
		{ FAIL_EXT('A', 'P', '3'), decode_ap3 },
		{ FAIL_EXT('I', 'L', 'C'), decode_ap3 },
		{ FAIL_EXT('R', 'I', 'P'), decode_rip },
		{ FAIL_EXT('F', 'N', 'T'), decode_fnt },
		{ FAIL_EXT('S', 'X', 'S'), decode_sxs },
		{ FAIL_EXT('M', 'C', 'P'), decode_mcp },
		{ FAIL_EXT('G', 'H', 'G'), decode_ghg }
	}, *ph;

	if (atari_palette == NULL)
		atari_palette = jakub_act;
	ext = get_packed_ext(filename);
	for (ph = handlers; ph < handlers + sizeof(handlers) / sizeof(handlers[0]); ph++) {
		if (ph->ext == ext) {
			abool result = ph->func(
				image, image_len, atari_palette,
				image_info, pixels);
			if (!result)
				return FALSE;
			rgb_to_palette(pixels, image_info->height * image_info->width,
				palette, &image_info->colors);
			return TRUE;
		}
	}
	return FALSE;
}
