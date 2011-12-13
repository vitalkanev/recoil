/*
 * fail.c - FAIL library functions
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "fail.h"

#include "palette.h"

#define FAIL_MODE_REAL11 27
#define FAIL_MODE_15PF0FIRST 16
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
			case FAIL_MODE_REAL11:
				{
					int hu = b << (i & 4) & 0xF0;
					frame[dest_pos + x] = hu == 0 ? color_regs[0] & 0xF0 : (color_regs[0] & 0xFE) | hu;
				}
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
			case FAIL_MODE_15PF0FIRST: /* 4 regs, PF0 first */
				frame[dest_pos + x] = color_regs[((b >> (~i & 6)) - 1) & 3] & 0xFE;
				break;
			case FAIL_MODE_CIN15: /* 4 * 256 regs, only first 192 of each 256-byte group are used */
				frame[dest_pos + x] = color_regs[(b >> (~i & 6) & 3) * 256 + y] & 0xFE;
				break;
			case FAIL_MODE_MULTIRIP:
				col = b >> (~i & 4) & 0x0F;
				frame[dest_pos + x] = col == 0 ? 0 : color_regs[gr10_to_reg[col] + (y / 2) * 8 - 1] & 0xFE;
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
static void frames_to_rgb_3(
	const byte frame1[], const byte frame2[], const byte frame3[],
	int frame_size,
	const byte atari_palette[],
	byte pixels[])
{
	int i;
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
	byte *temp_palette = (byte *) malloc(FAIL_PIXELS_MAX);
	if (temp_palette == NULL) {
		*colors = 65536;
		return FALSE;
	}

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
	if (palette == NULL || *colors > 256) {
		free(temp_palette);
		return FALSE;
	}

	memcpy(palette, temp_palette, FAIL_PALETTE_MAX);
	for (i = 0; i < pixel_count; i++) {
		int index;
		if (find_rgb_color(temp_palette, *colors, rgb_to_int(pixels + i * 3), &index))
			pixels[i] = index;
	}
	free(temp_palette);
	/* pad palette with 0s */
	for (i = *colors * 3; i < FAIL_PALETTE_MAX; i++)
		palette[i] = 0;

	return TRUE;
}

static abool unpack_koala(const byte data[], int data_len, int cprtype, byte unpacked_data[], int unpacked_data_len)
{
	int data_offset;
	int unpacked_offset;
	switch (cprtype) {
	case 0:
		if (data_len != unpacked_data_len)
			return FALSE;
		memcpy(unpacked_data, data, unpacked_data_len);
		return TRUE;
	case 1:
	case 2:
		break;
	default:
		return FALSE;
	}
	data_offset = 0;
	unpacked_offset = 0;
	for (;;) {
		int b;
		int len;
		if (data_offset >= data_len)
			return FALSE;
		b = data[data_offset++];
		len = b & 0x7f;
		if (len == 0) {
			if (data_offset + 1 >= data_len)
				return FALSE;
			len = (data[data_offset] << 8) + data[data_offset + 1];
			if (len == 0)
				return FALSE;
			data_offset += 2;
		}
		if (b >= 0x80)
			b = -1;
		else {
			if (data_offset >= data_len)
				return FALSE;
			b = data[data_offset++];
		}
		do {
			if (b < 0) {
				if (data_offset >= data_len)
					return FALSE;
				unpacked_data[unpacked_offset] = data[data_offset++];
			}
			else
				unpacked_data[unpacked_offset] = (byte) b;
			/* return if last byte written */
			if (unpacked_offset >= unpacked_data_len - 1)
				return TRUE;
			if (cprtype == 2)
				unpacked_offset++;
			else {
				unpacked_offset += 80;
				if (unpacked_offset >= unpacked_data_len) {
					/* if in line 192, back to odd lines in the same column;
					   if in line 193, go to even lines in the next column */
					unpacked_offset -= (unpacked_offset < unpacked_data_len + 40) ? unpacked_data_len - 40 : unpacked_data_len + 39;
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

typedef struct {
	int count[16]; /* count[n] == number of codes of bit length n */
	byte values[256]; /* values sorted by code length */
} FanoTree;

static void unpack_rip_create_fano_tree(const byte *src, int n, FanoTree *tree)
{
	int i;
	int pos = 0;
	int positions[16];
	for (i = 0; i < 16; i++)
		tree->count[i] = 0;
	for (i = 0; i < n; i++) {
		int bits = src[i >> 1];
		bits = (i & 1) == 0 ? bits >> 4 : bits & 0xf;
		tree->count[bits]++;
	}
	for (i = 0; i < 16; i++) {
		positions[i] = pos;
		pos += tree->count[i];
	}
	for (i = 0; i < n; i++) {
		int bits = src[i >> 1];
		bits = (i & 1) == 0 ? bits >> 4 : bits & 0xf;
		tree->values[positions[bits]++] = (byte) i;
	}
}

typedef struct {
	byte bits; /* sliding left with a trailing 1 */
	const byte *bytes;
	int length;
	int offset;
} BitStream;

static int unpack_rip_get_bit(BitStream *s)
{
	int bits = s->bits;
	if (bits == 0x80) {
		if (s->offset >= s->length)
			return -1;
		bits = s->bytes[s->offset++] * 2 + 1;
	}
	else
		bits <<= 1;
	s->bits = (byte) bits;
	return bits >> 8;
}

static int unpack_rip_get_code(BitStream *s, const FanoTree *tree)
{
	int p = tree->count[0];
	int i = 0;
	int bits;
	for (bits = 1; bits < 16; bits++) {
		int n = tree->count[bits];
		int bit = unpack_rip_get_bit(s);
		if (bit == -1)
			return -1;
		i = i * 2 + bit;
		if (i < n)
			return tree->values[p + i];
		p += n;
		i -= n;
	}
	return -1;
}

static abool unpack_rip(const byte data[], int data_len, byte unpacked_data[], int unpacked_len)
{
	FanoTree length_tree;
	FanoTree distance_tree;
	FanoTree literal_tree;
	BitStream stream = { 0x80, data, data_len, 16 + 288 };
	int unpacked_offset;

	/* "PCK" header (16 bytes) */
	if (data_len < 304 || data[0] != 'P' || data[1] != 'C' || data[2] != 'K')
		return FALSE;

	/* 288 bytes Shannon-Fano bit lengths */
	unpack_rip_create_fano_tree(data + 16, 64, &length_tree);
	unpack_rip_create_fano_tree(data + 16 + 32, 256, &distance_tree);
	unpack_rip_create_fano_tree(data + 16 + 32 + 128, 256, &literal_tree);

	/* LZ77 */
	for (unpacked_offset = 0; unpacked_offset < unpacked_len; ) {
		switch (unpack_rip_get_bit(&stream)) {
		case -1:
			return FALSE;
		case 0:
			unpacked_data[unpacked_offset++] = (byte) unpack_rip_get_code(&stream, &literal_tree);
			break;
		case 1:
			{
				int distance = unpack_rip_get_code(&stream, &distance_tree) + 2;
				int len;
				if (distance > unpacked_offset)
					return FALSE;
				len = unpack_rip_get_code(&stream, &length_tree) + 2;
				do {
					unpacked_data[unpacked_offset] = unpacked_data[unpacked_offset - distance];
					unpacked_offset++;
				} while (--len > 0);
				break;
			}
		}
	}

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

static abool decode_gr8_gr9(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[],
	int dest_mode, const byte color_regs[])
{
	int offset = 0;
	int frame_len;
	byte frame[320 * 240];

	/* optional binary file header */
	if (parse_binary_header(image, &frame_len)
	 && frame_len == image_len - 6)
		offset = 6;

	image_info->width = 320;
	image_info->height = (image_len - offset) / 40;
	image_info->original_height = image_info->height;

	if (/*(image_len - offset) % 40 != 0 || */image_info->height > 240)
		return FALSE;

	decode_video_memory(
		image, color_regs,
		offset, 40, 0, 1, 0, 40, image_info->height, dest_mode,
		frame);

	frame_to_rgb(frame, image_info->height * 320, atari_palette, pixels);

	return TRUE;
}

static const byte gr8_color_regs[] = { 0x00, 0x0e };

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

static const byte gr9_color_regs[] = { 0x00 };

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
		pixels, 9, gr9_color_regs);
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
	int frame1_len;
	int frame2_len;
	byte frame1[320 * 240];
	byte frame2[320 * 240];

	if (parse_binary_header(image, &frame1_len)
	 && frame1_len > 0
	 && frame1_len * 2 + 12 == image_len
	 && frame1_len % 40 == 0
	 && parse_binary_header(image + frame1_len + 6, &frame2_len)
	 && frame2_len == frame1_len) {
		/* hip image with binary file headers */
		image_info->height = frame1_len / 40;
		if (image_info->height > 240)
			return FALSE;
		decode_video_memory(
			image, hip_color_regs,
			12 + frame1_len, 40, 0, 1, -1, 40, image_info->height, 9,
			frame2);
		decode_video_memory(
			image, hip_color_regs,
			6, 40, 0, 1, +1, 40, image_info->height, 10,
			frame1);
	}
	else {
		/* hip image with gr10 palette */
		image_info->height = image_len / 80;
		if (image_info->height == 0 || image_info->height > 240)
			return FALSE;
		decode_video_memory(
			image, hip_color_regs,
			0, 40, 0, 1, -1, 40, image_info->height, 9,
			frame1);
		decode_video_memory(
			image, image_len % 80 == 9 ? image + image_len - 9 : hip_color_regs,
			40 * image_info->height, 40, 0, 1, +1, 40, image_info->height, 10,
			frame2);
	}

	image_info->width = 320;
	image_info->original_width = 160;
	image_info->original_height = image_info->height;

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
	const byte *color_regs;
	int mode;
	byte frame[320 * 240];

	if (image_len == 15872) {
		/* AtariGraphics file format.
		AtariGraphics was included on a cartridge with the Atari Light Pen.
		Its files have no default extension.
		The format is: 7680 bytes picture, followed by PF0,PF1,PF2,BAK colors,
		followed by 508 bytes of padding, followed by 7680 bytes of fill map.
		The fill map allows flood filling areas previously filled with a pattern.
		It is a copy of the picture, except that areas filled with a pattern
		are filled with a solid color here. Also, the fill map has inverted bits
		compared to the picture, don't know why. */
		image_len = 7680;
		color_regs = image + 7680;
		mode = FAIL_MODE_15PF0FIRST;
	}
	else if (image_len % 40 == 4) {
		color_regs = image + image_len - 4;
		mode = 15;
	}
	else if (image_len % 40 == 0) {
		color_regs = mic_color_regs;
		mode = 15;
	}
	else
		return FALSE;

	image_info->width = 320;
	image_info->height = image_len / 40;
	if (image_info->height > 240)
		return FALSE;
	image_info->original_width = 320;
	image_info->original_height = image_info->height;

	decode_video_memory(
		image, color_regs,
		0, 40, 0, 1, 0, 40, image_info->height, mode,
		frame);

	frame_to_rgb(frame, image_info->height * 320, atari_palette, pixels);

	return TRUE;
}

static abool is_koala_header(const byte image[], int image_len)
{
	return image_len >= 26
		&& image[0] == 0xff && image[1] == 0x80
		&& image[2] == 0xc9 && image[3] == 0xc7
		&& image[4] >= 26 && image[4] < image_len
		&& image[5] == 0
		&& image[6] == 1 && image[8] == 0x0e
		&& image[9] == 0 && image[10] == 40
		&& image[11] == 0 && image[12] == 192
		&& image[20] == 0 && image[21] == 0;
}

static abool decode_pic(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	byte unpacked_image[7680 + 4];

	if (!is_koala_header(image, image_len)) {
		/* some images with .pic extension are
		   in micropainter format */
		if (image_len == 7684 || image_len == 7680) {
			return decode_mic(
				image, image_len, atari_palette,
				image_info, pixels);
		}
		return FALSE;
	}

	if (!unpack_koala(
		image + image[4] + 1, image_len - image[4] - 1,
		image[7], unpacked_image, 7680))
		return FALSE;

	unpacked_image[7680] = image[17];
	unpacked_image[7681] = image[13];
	unpacked_image[7682] = image[14];
	unpacked_image[7683] = image[15];

	return decode_mic(
		unpacked_image, 7684, atari_palette,
		image_info, pixels);
}

static abool decode_cpr(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	static const byte cpr_color_regs[] = { 0x0c, 0x00 };
	byte unpacked_image[7680];

	if (!unpack_koala(
		image + 1, image_len - 1,
		image[0], unpacked_image, 7680))
		return FALSE;

	image_info->original_width = 320;
	return decode_gr8_gr9(
		unpacked_image, 7680, atari_palette,
		image_info,
		pixels, 8, cpr_color_regs);
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

static abool decode_int(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	byte frame1[320 * 240];
	byte frame2[320 * 240];

	if (image[0] != 'I' || image[1] != 'N' || image[2] != 'T'
	 || image[3] != '9' || image[4] != '5' || image[5] != 'a'
	 || image[6] == 0 || image[6] > 40
	 || image[7] == 0 || image[7] > 239
	 || image[8] != 0x0f || image[9] != 0x2b
	 || 18 + image[6] * image[7] * 2 != image_len)
		return decode_inp(image, image_len,
			atari_palette, image_info, pixels);

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

static abool decode_cin(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	const byte *color_regs;
	int mode;
	byte frame1[320 * 192];
	byte frame2[320 * 192];
	if (image_len == 15360) {
		color_regs = mic_color_regs;
		mode = 15;
	}
	else if (image_len == 16384) {
		color_regs = image + 0x3c00;
		mode = FAIL_MODE_CIN15;
	}
	else
		return FALSE;

	image_info->width = 320;
	image_info->height = 192;
	image_info->original_width = 160;
	image_info->original_height = 192;

	decode_video_memory(
		image, color_regs,
		40, 80, 1, 2, 0, 40, 96, mode,
		frame1);

	decode_video_memory(
		image, NULL,
		7680, 80, 0, 2, 0, 40, 96, 11,
		frame1);

	decode_video_memory(
		image, color_regs,
		0, 80, 0, 2, 0, 40, 96, mode,
		frame2);

	decode_video_memory(
		image, NULL,
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
	byte frame1[320 * 240];
	byte frame2[320 * 240];

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
		image, NULL,
		9 + 2 * frame_len, line_len, 0, 2, -1, line_len, image[6], 11,
		frame1);

	/* odd frame, gr11 + gr10 */
	decode_video_memory(
		image, hip_color_regs,
		9 + frame_len, line_len, 1, 2, +1, line_len, image[6], 10,
		frame2);

	decode_video_memory(
		image, NULL,
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
		image, NULL,
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
		40, 80, 1, 2, 0, 40, 96, 9,
		frame1);

	decode_video_memory(
		image, NULL,
		image_len == 15360 ? 7680 : 8192, 80, 0, 2, 0, 40, 96, 11,
		frame1);

	decode_video_memory(
		image, hip_color_regs,
		0, 80, 0, 2, 0, 40, 96, 9,
		frame2);

	decode_video_memory(
		image, NULL,
		image_len == 15360 ? 7720 : 8232, 80, 1, 2, 0, 40, 96, 11,
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
	byte frame1[320 * 240];
	byte frame2[320 * 240];
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
		line_len = image[13];
		if (image[7] == 0x30)
			line_len += 4; /* multi rip: 8 bytes of palette per two lines */
		if (unpack_rip(image + hdr_len, data_len, unpacked_image, line_len * image[15]))
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
	case 0x0e:
		/* gr. 15 */
		{
			byte colors[4] = { image[32 + txt_len], image[28 + txt_len], image[29 + txt_len], image[30 + txt_len] };
			decode_video_memory(
				unpacked_image, colors,
				frame_len, line_len, 0, 1, 0, line_len, image_info->height,
				15, frame1);
		}
		frame_to_rgb(frame1, image_info->height * image_info->width,
			atari_palette, pixels);
		return TRUE;
	case 0x0f:
		/* gr. 8 */
		{
			byte colors[2] = { image[29 + txt_len], image[28 + txt_len] };
			decode_video_memory(
				unpacked_image, colors,
				frame_len, line_len, 0, 1, 0, line_len, image_info->height,
				8, frame1);
		}
		frame_to_rgb(frame1, image_info->height * image_info->width,
			atari_palette, pixels);
		return TRUE;
	case 0x4f:
		/* gr. 9 */
		decode_video_memory(
			unpacked_image, image + 32 + txt_len,
			frame_len, line_len, 0, 1, 0, line_len, image_info->height,
			9, frame1);
		frame_to_rgb(frame1, image_info->height * image_info->width,
			atari_palette, pixels);
		return TRUE;
	case 0x8f:
		/* gr. 10 */
		decode_video_memory(
			unpacked_image, image + 24 + txt_len,
			frame_len, line_len, 0, 1, 0, line_len, image_info->height,
			10, frame1);
		frame_to_rgb(frame1, image_info->height * image_info->width,
			atari_palette, pixels);
		return TRUE;
	case 0xcf:
		/* gr. 11 */
		decode_video_memory(
			unpacked_image, image + 32 + txt_len,
			frame_len, line_len, 0, 1, 0, line_len, image_info->height,
			FAIL_MODE_REAL11, frame1);
		frame_to_rgb(frame1, image_info->height * image_info->width,
			atari_palette, pixels);
		return TRUE;
	case 0x10:
		/* interlaced gr. 15 */
		decode_video_memory(
			unpacked_image, image + 28 + txt_len,
			0, line_len * 2, 0, 2, 0, line_len, image_info->height / 2,
			15, frame1);
		decode_video_memory(
			unpacked_image, image + 24 + txt_len,
			40, line_len * 2, 1, 2, 0, line_len, image_info->height / 2,
			15, frame1);
		decode_video_memory(
			unpacked_image, image + 24 + txt_len,
			frame_len, line_len * 2, 0, 2, 0, line_len, image_info->height / 2,
			15, frame2);
		decode_video_memory(
			unpacked_image, image + 28 + txt_len,
			frame_len + 40, line_len * 2, 1, 2, 0, line_len, image_info->height / 2,
			15, frame2);
		break;
	case 0x20:
		/* hip, rip */
		decode_video_memory(
			unpacked_image, image + 24 + txt_len,
			0, line_len, 0, 1, +1, line_len, image_info->height,
			10, frame1);
		decode_video_memory(
			unpacked_image, hip_color_regs,
			frame_len, line_len, 0, 1, -1, line_len, image_info->height,
			9, frame2);
		break;
	case 0x30:
		/* multi rip */
		decode_video_memory(
			unpacked_image, unpacked_image + 2 * frame_len,
			0, line_len, 0, 1, +1, line_len, image_info->height,
			FAIL_MODE_MULTIRIP, frame1);
		decode_video_memory(
			unpacked_image, hip_color_regs,
			frame_len, line_len, 0, 1, -1, line_len, image_info->height,
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

	image_info->width = 320;
	image_info->height = 200;
	image_info->original_width = 160;
	image_info->original_height = 200;

	decode_video_memory(
		image, image + 16000,
		0, 80, 0, 2, 0, 40, 100, FAIL_MODE_15PF0FIRST,
		frame1);

	decode_video_memory(
		image, image + 16004,
		40, 80, 1, 2, 0, 40, 100, FAIL_MODE_15PF0FIRST,
		frame1);

	decode_video_memory(
		image, image + 16004,
		8000, 80, 0, 2, 0, 40, 100, FAIL_MODE_15PF0FIRST,
		frame2);

	decode_video_memory(
		image, image + 16000,
		8040, 80, 1, 2, 0, 40, 100, FAIL_MODE_15PF0FIRST,
		frame2);

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
	/* round up to 8 pixels */
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

static abool decode_hr2(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	byte frame1[320 * 200];
	byte frame2[320 * 200];
	if (image_len != 16006)
		return FALSE;
	image_info->original_width = image_info->width = 320;
	image_info->original_height = image_info->height = 200;

	decode_video_memory(
		image, image + 16000,
		0, 40, 0, 1, 0, 40, 200, 8,
		frame1);

	decode_video_memory(
		image, image + 16002,
		8000, 40, 0, 1, 0, 40, 200, 15,
		frame2);

	frames_to_rgb(frame1, frame2, 320 * 200, atari_palette, pixels);
	return TRUE;
}

static abool decode_mch(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	int chars_per_line;
	int char_offset;
	byte frame[352 * 240];
	int y;
	switch (image_len) {
	case 30 * 32 * 9 + 240 * 5:
		image_info->original_width = 128;
		chars_per_line = 32;
		char_offset = 0;
		break;
	case 30 * 40 * 9 + 240 * 5:
		image_info->original_width = 160;
		chars_per_line = 40;
		char_offset = 0;
		break;
	case 30 * 48 * 9 + 240 * 5:
		image_info->original_width = 176;
		chars_per_line = 48;
		char_offset = 3;
		break;
	default:
		return FALSE;
	}
	image_info->width = 2 * image_info->original_width;
	image_info->height = image_info->original_height = 240;
	for (y = 0; y < 240; y++) {
		const byte *colors = image + image_len - 240 * 5 + y;
		int x;
		for (x = 0; x < image_info->original_width; x++) {
			const byte *p = image + ((y >> 3) * chars_per_line + (x >> 2) + char_offset) * 9;
			int color = (p[1 + (y & 7)] >> (2 * (~x & 3))) & 3;
			if (color == 3 && (p[0] & 0x80) != 0)
				color = 4;
			frame[y * image_info->width + 2 * x + 1] =
				frame[y * image_info->width + 2 * x] = colors[color * 240] & 0xfe;
		}
	}
	frame_to_rgb(frame, image_info->width * 240, atari_palette, pixels);
	return TRUE;
}

static abool decode_ige(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	byte frame1[256 * 96];
	byte frame2[256 * 96];
	if (image_len != 6160 || image[0] != 0xff || image[1] != 0xff
	 || image[2] != 0xf6 || image[3] != 0xa3 || image[4] != 0xff || image[5] != 0xbb
	 || image[6] != 0xff || image[7] != 0x5f)
		return FALSE;
	image_info->width = 256;
	image_info->height = 96;
	image_info->original_width = 128;
	image_info->original_height = 96;
	decode_video_memory(
		image, image + 8,
		0x10, 32, 0, 1, 0, 32, 96,
		15, frame1);
	decode_video_memory(
		image, image + 12,
		0xc10, 32, 0, 1, 0, 32, 96,
		15, frame2);
	frames_to_rgb(frame1, frame2, 256 * 96, atari_palette, pixels);
	return TRUE;
}

/* serves both 256 and AP2 formats */
static abool decode_256(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	byte frame[320 * 192];

	if (image_len != 7680 && image_len != 7684)
		return FALSE;

	image_info->width = 320;
	image_info->height = 192;
	image_info->original_width = 80;
	image_info->original_height = 192;

	decode_video_memory(
		image, hip_color_regs,
		3840, 40, 1, 2, 0, 40, 96, 9,
		frame);

	decode_video_memory(
		image, NULL,
		0, 40, 0, 2, 0, 40, 96, 11,
		frame);

	frame_to_rgb(frame, image_info->height * image_info->width,
		atari_palette, pixels);

	return TRUE;
}

static abool decode_jgp(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	byte ordered_bytes[2048];
	byte frame[256 * 64];
	int i;

	if (image_len != 2054 || image[0] != 0xff || image[1] != 0xff
	 || image[4] + image[5] * 256 - image[2] - image[3] * 256 != 2047)
		return FALSE;

	image_info->width = 256;
	image_info->height = 64;
	image_info->original_width = 128;
	image_info->original_height = 64;

	for (i = 0; i < 2048; i++) {
		ordered_bytes[
			((i & 0x300) << 1)
			+ ((i >> 2) & 0x100)
			+ ((i & 7) << 5)
			+ ((i >> 3) & 0x1f)
		] = image[6 + i];
	}

	decode_video_memory(
		ordered_bytes, mic_color_regs,
		0, 32, 0, 1, 0, 32, 64, 15, frame);

	frame_to_rgb(frame, 256 * 64, atari_palette, pixels);

	return TRUE;
}

static abool decode_pzm(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	if (image_len == 15360 || image_len == 15362)
		return decode_ap3(image, 15360,
			atari_palette, image_info, pixels);

	return FALSE;
}

static abool decode_ist(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	int y;
	byte frame1[320 * 200];
	byte frame2[320 * 200];

	if (image_len != 17184)
		return FALSE;

	image_info->width = 320;
	image_info->height = 200;
	image_info->original_width = 160;
	image_info->original_height = 200;

	for (y = 0; y < 200; y++) {
		byte color_regs[4] = { image[0x4000 + y], image[0x40c8 + y], image[0x4190 + y], image[0x4258 + y] };
		decode_video_memory(
			image, color_regs,
			16 + 40 * y, 40, y, 1, 0, 40, 1, 15,
			frame1);

		decode_video_memory(
			image, color_regs,
			0x2010 + 40 * y, 40, y, 1, 0, 40, 1, 15,
			frame2);
	}

	frames_to_rgb(frame1, frame2, image_info->height * 320, atari_palette, pixels);

	return TRUE;
}

static abool decode_raw(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	byte frame1[320 * 192];
	byte frame2[320 * 192];

	if (image_len != 15372 || image[0] != 'X' || image[1] != 'L' || image[2] != 'P' || image[3] != 'B')
		return FALSE;

	image_info->width = 320;
	image_info->height = 192;
	image_info->original_width = 160;
	image_info->original_height = 192;

	decode_video_memory(
		image, image + 0x3c04,
		4, 40, 0, 1, 0, 40, 192, FAIL_MODE_15PF0FIRST,
		frame1);

	decode_video_memory(
		image, image + 0x3c08,
		0x1e04, 40, 0, 1, 0, 40, 192, FAIL_MODE_15PF0FIRST,
		frame2);

	frames_to_rgb(frame1, frame2, 320 * 192, atari_palette, pixels);

	return TRUE;
}

static abool decode_mgp(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	byte color_regs[4];
	byte frame[320 * 192];
	int x;
	int y;

	if (image_len != 3845)
		return FALSE;

	image_info->width = 320;
	image_info->height = 192;
	image_info->original_width = 160;
	image_info->original_height = 96;

	color_regs[0] = image[4];
	color_regs[1] = image[0];
	color_regs[2] = image[1];
	color_regs[3] = image[2];

	for (y = 0; y < 96; y++) {
		if (image[5] < 4) {
			/* Rainbow effect :)
			The constant 16 is arbitrary here. For correct animation it should decrease every frame. */
			color_regs[image[5]] = 16 + y;
		}
		decode_video_memory(
			image, color_regs,
			6 + 40 * y, 0, y * 2, 1, 0, 40, 2, 15,
			frame);
	}
	/* The file is missing the last byte of the graphics.
	   Let's fill it with background so that at least it's not random. */
	for (x = 0; x < 8; x++)
		frame[320 * 190 + 312 + x] = frame[320 * 191 + 312 + x] = color_regs[0] & 0xfe;

	frame_to_rgb(frame, 320 * 192, atari_palette, pixels);
	return TRUE;
}

static abool decode_rgb(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	int title_len;
	int width;
	int bytes_per_line;
	int image_offset;
	abool hi_nibble;
	int x;
	int count;
	abool literal = FALSE;
	int r = 0;
	int g = 0;
	int b = 0;
	byte screens[3][40 * 192];
	int i;
	byte frames[3][320 * 192];
	if (image_len < 9 || image[0] != 'R' || image[1] != 'G' || image[2] != 'B' || image[3] != '1')
		return FALSE;
	title_len = image[4];
	width = image[6 + title_len];
	image_info->original_height = image_info->height = image[7 + title_len];
	if (width == 0 || width > 80 || image_info->height == 0 || image_info->height > 192 || image[8 + title_len] != 1)
		return FALSE;
	bytes_per_line = (width + 1) >> 1;
	image_info->width = bytes_per_line << 3;

	image_offset = 9 + title_len;
	hi_nibble = TRUE;
	count = 1;
	for (x = 0; x < width; x++) {
		int y;
		for (y = 0; y < image_info->height; y++) {
			int screen_offset = y * bytes_per_line + (x >> 1);
#define GET_NIBBLE(dest) \
			if (hi_nibble) { \
				if (image_offset >= image_len) \
					return FALSE; \
				dest = image[image_offset] >> 4; \
				hi_nibble = FALSE; \
			} \
			else { \
				dest = image[image_offset++] & 0xf; \
				hi_nibble = TRUE; \
			}

			if (--count == 0) {
				GET_NIBBLE(count);
				if (count >= 8) {
					literal = TRUE;
					count &= 7;
				}
				else
					literal = FALSE;
				if (count == 0) {
					GET_NIBBLE(count);
					count += 7;
				}
				if (!literal) {
					GET_NIBBLE(r);
					GET_NIBBLE(g);
					GET_NIBBLE(b);
					count++;
				}
			}
			if (literal) {
				GET_NIBBLE(r);
				GET_NIBBLE(g);
				GET_NIBBLE(b);
			}
#undef GET_NIBBLE

			if ((x & 1) == 0) {
				screens[0][screen_offset] = r << 4;
				screens[1][screen_offset] = g << 4;
				screens[2][screen_offset] = b << 4;
			}
			else {
				screens[0][screen_offset] |= r;
				screens[1][screen_offset] |= g;
				screens[2][screen_offset] |= b;
			}
		}
	}

	switch (image[5 + title_len]) {
	case 9:
		image_info->original_width = bytes_per_line << 1;
		for (i = 0; i < 3; i++) {
			static const byte gr9_colors[3] = { 0x30, 0xc0, 0x70 };
			decode_video_memory(
				screens[i], gr9_colors + i,
				0, bytes_per_line, 0, 1, 0, bytes_per_line, image_info->height, 9,
				frames[i]);
		}
		break;
	case 15:
		image_info->original_width = bytes_per_line << 2;
		for (i = 0; i < 3; i++) {
			static const byte gr15_colors[3][4] = {
				{ 0x00, 0x34, 0x3a, 0x3e },
				{ 0x00, 0xc4, 0xca, 0xce },
				{ 0x00, 0x74, 0x7a, 0x7e } };
			decode_video_memory(
				screens[i], gr15_colors[i],
				0, bytes_per_line, 0, 1, 0, bytes_per_line, image_info->height, 15,
				frames[i]);
		}
		break;
	default:
		return FALSE;
	}

	frames_to_rgb_3(frames[0], frames[1], frames[2], image_info->width * image_info->height, atari_palette, pixels);
	return TRUE;
}

static abool decode_wnd(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	static const byte blazing_paddles_default_color_regs[] = { 0x00, 0x46, 0x88, 0x0e };
	byte frame[320 * 192];
	int bytes_per_line;
	int height;

	if (image_len != 3072)
		return FALSE;

	/* first byte = width in pixels -1 */
	bytes_per_line = (image[0] >> 2) + 1;
	height = image[1];
	if (bytes_per_line > 40 || height == 0 || height > 192 || bytes_per_line * height > 3070)
		return FALSE;
	/* round up to 4 pixels */
	image_info->width = bytes_per_line * 8;
	image_info->height = height;
	image_info->original_width = bytes_per_line * 4;
	image_info->original_height = height;

	decode_video_memory(
		image, blazing_paddles_default_color_regs,
		2, bytes_per_line, 0, 1, 0, bytes_per_line, height, 15,
		frame);

	frame_to_rgb(frame, image_info->width * height, atari_palette, pixels);
	return TRUE;
}

typedef struct {
	int left;
	int top;
	int right;
	int bottom;
} BoundingBox;

static abool get_blazing_paddles_vector_bounding_box(
	const byte image[], int image_len, int index, int start_address,
	BoundingBox *box)
{
	int image_offset;
	int x;
	int y;
	if (index * 2 + 1 >= image_len)
		return FALSE;
	image_offset = image[index * 2] + (image[index * 2 + 1] << 8) - start_address;
	if (image_offset < 0)
		return FALSE;
	x = 0;
	y = 0;
	box->left = 0;
	box->top = 0;
	box->right = 0;
	box->bottom = 0;
	while (image_offset < image_len) {
		int control = image[image_offset++];
		int len;
		if (control == 0x08)
			return TRUE;
		/* bits 7-4: length-1 */
		len = (control >> 4) + 1;
		/* bits 1-0: direction */
		switch (control & 3) {
		case 0: /*right */
			x += len;
			if (box->right < x)
				box->right = x;
			break;
		case 1: /* left */
			x -= len;
			if (box->left > x)
				box->left = x;
			break;
		case 2: /* up */
			y -= len;
			if (box->top > y)
				box->top = y;
			break;
		case 3: /* down */
			y += len;
			if (box->bottom < y)
				box->bottom = y;
			break;
		}
	}
	return FALSE;
}

static abool draw_blazing_paddles_vector(
	const byte image[], int image_len, int index, int start_address,
	byte frame[], int frame_offset, int frame_width)
{
	int image_offset;
	if (index * 2 + 1 >= image_len)
		return FALSE;
	image_offset = image[index * 2] + (image[index * 2 + 1] << 8) - start_address;
	if (image_offset < 0)
		return FALSE;
	while (image_offset < image_len) {
		int control = image[image_offset++];
		if (control == 0x08)
			return TRUE;
		/* bits 7-4: length-1 */
		for (; control >= 0; control -= 16) {
			/* bit 2: pen up */
			if ((control & 4) == 0) {
				frame[frame_offset] = 0x0e;
				frame[frame_offset + 1] = 0x0e;
			}
			/* bits 1-0: direction */
			switch (control & 3) {
			case 0: /*right */
				frame_offset += 2;
				break;
			case 1: /* left */
				frame_offset -= 2;
				break;
			case 2: /* up */
				frame_offset -= frame_width;
				break;
			case 3: /* down */
				frame_offset += frame_width;
				break;
			}
		}
	}
	return FALSE;
}

static abool decode_blazing_paddles_vectors(
	const byte image[], int image_len, int start_address,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	byte frame[320 * 240];
	int x = 0;
	int y = 0;
	int i;
	int line_i = 0;
	int line_top = 0;
	int line_bottom = 0;
	int xs[256];
	int ys[256];

	/* The file contains several independent shapes.
	I layout them in reading order, so that they don't overlap,
	the baselines are aligned and everything fits in 160x240. */
	image_info->original_width = 0;
	for (i = 0; i < 256; i++) {
		BoundingBox box;
		int width;
		if (!get_blazing_paddles_vector_bounding_box(image, image_len, i, start_address, &box))
			break;
		width = box.right - box.left + 2; /* +1 because box.right is inclusive, +1 for space */
		if (x + width > 160) {
			/* new line */
			y -= line_top;
			while (line_i < i)
				ys[line_i++] = y;
			if (image_info->original_width < x)
				image_info->original_width = (x + 3) & ~3; /* round up to 4 pixels */
			x = 0;
			y += line_bottom + 2; /* +1 because box.bottom is inclusive, +1 for space */
			line_top = box.top;
			line_bottom = box.bottom;
		}
		/* place this shape at x,y */
		xs[i] = x - box.left;
		x += width;
		if (line_top > box.top)
			line_top = box.top;
		if (line_bottom < box.bottom)
			line_bottom = box.bottom;
	}
	y -= line_top;
	while (line_i < i)
		ys[line_i++] = y;
	if (image_info->original_width < x)
		image_info->original_width = (x + 3) & ~3; /* round up to 4 pixels */
	y += line_bottom + 1; /* +1 because box.bottom is inclusive */
	if (i == 0 || y > 240)
		return FALSE;
	image_info->width = image_info->original_width * 2;
	image_info->height = image_info->original_height = y;
	memset(frame, 0, image_info->width * image_info->height);

	/* draw shapes */
	for (i = 0; i < 256; i++) {
		if (!draw_blazing_paddles_vector(image, image_len, i, start_address, frame, ys[i] * image_info->width + xs[i] * 2, image_info->width))
			break;
	}

	frame_to_rgb(frame, image_info->width * image_info->height, atari_palette, pixels);
	return TRUE;
}

static abool decode_chr(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	return image_len == 3072 && decode_blazing_paddles_vectors(image, image_len, 0x7000, atari_palette, image_info, pixels);
}

static abool decode_shp(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	return image_len == 1024 && decode_blazing_paddles_vectors(image, image_len, 0x7c00, atari_palette, image_info, pixels);
}

static abool decode_mbg(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	byte frame[512 * 256];
	if (image_len != 16384)
		return FALSE;

	image_info->width = image_info->original_width = 512;
	image_info->height = image_info->original_height = 256;

	decode_video_memory(
		image, gr8_color_regs,
		0, 64, 0, 1, 0, 64, 256, 8,
		frame);

	frame_to_rgb(frame, 512 * 256, atari_palette, pixels);

	return TRUE;
}

static abool decode_fwa(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	byte frame[320 * 192];
	byte color_regs[4];
	int dl_offset;
	int dli_offset;
	int y;

	if (image_len < 0x1f18 || image[0] != 0xfe || image[1] != 0xfe
	 || image[6] != 0x70 || image[7] != 0x70 || image[8] != 0x70 || image[0xb] != 0x50 || image[0x73] != 0x60 || image[0xcd] != 0x41
	 || 0x1f18 + image[0x1f16] + (image[0x1f17] << 8) != image_len)
		return FALSE;

	color_regs[0] = image[2];
	color_regs[1] = image[3];
	color_regs[2] = image[4];
	color_regs[3] = image[5];
	dl_offset = 9;
	dli_offset = 0x1f18;
	for (y = 0; y < 192; y++) {
		int dl_instr;
		decode_video_memory(
			image, color_regs,
			0x106 + 40 * y + (y >= 102 ? 16 : 0), 40, y, 1, 0, 40, 1, 15,
			frame);
		dl_instr = image[dl_offset];
		if (dl_offset == 9 || dl_offset == 0x71) {
			if ((dl_instr & 0x7f) != 0x4e || image[dl_offset + 1] != 0)
				return FALSE;
			dl_offset += 3;
		}
		else {
			if ((dl_instr & 0x7f) != 0x0e)
				return FALSE;
			dl_offset++;
		}
		if (dl_instr >= 0x80) {
			byte a;
			if (dli_offset + 14 > image_len || image[dli_offset] != 0x48 || image[dli_offset + 1] != 0x8a || image[dli_offset + 2] != 0x48
			 || image[dli_offset + 3] != 0xa9 || image[dli_offset + 5] != 0x8d || image[dli_offset + 6] != 0x0a || image[dli_offset + 7] != 0xd4)
				return FALSE;
			a = image[dli_offset + 4];
			dli_offset += 8;
			while (image[dli_offset] != 0x20) {
				switch (image[dli_offset]) {
				case 0xa9:
					a = image[dli_offset + 1];
					dli_offset += 2;
					break;
				case 0x8d:
					if (image[dli_offset + 2] != 0xd0)
						return FALSE;
					switch (image[dli_offset + 1]) {
					case 0x16:
						color_regs[1] = a;
						break;
					case 0x17:
						color_regs[2] = a;
						break;
					case 0x18:
						color_regs[3] = a;
						break;
					case 0x1a:
						color_regs[0] = a;
						break;
					default:
						return FALSE;
					}
					dli_offset += 3;
					break;
				default:
					return FALSE;
				}
				if (dli_offset + 3 > image_len)
					return FALSE;
			}
			if (image[dli_offset + 1] != 0xca || image[dli_offset + 2] != 0x06)
				return FALSE;
			dli_offset += 3;
		}
	}

	image_info->width = 320;
	image_info->height = 192;
	image_info->original_width = 160;
	image_info->original_height = 192;
	frame_to_rgb(frame, 320 * 192, atari_palette, pixels);
	return TRUE;
}

static abool decode_rm(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[], int mode)
{
	byte unpacked_image[7680];
	const byte *raw_image;
	int colors_offset;
	int dli_offset;
	abool dli_present[192];
	int y;
	int i;
	byte color_regs[9];
	byte frame[320 * 192];

	if (is_koala_header(image, image_len - 464)) {
		/* File written by a Rambrandt plugin ("DOS module").
		   Documentation suggests RM0-RM4 extensions. */
		if (!unpack_koala(
			image + image[4] + 1, image_len - image[4] - 465,
			image[7], unpacked_image, mode == 0 ? 3840 : 7680))
			return FALSE;
		raw_image = unpacked_image;
		colors_offset = image_len - 464;
		dli_offset = image_len - 384;
	}
	else if (image_len == 8192) {
		/* Rambrandt native raw format.
		   Rambrandt writes directly to disk using its own filesystem:
		   10 pictures per disk, 32 characters filename (no extension).
		   We assume RM0-RM4 extensions even though it's a different format. */
		raw_image = image;
		colors_offset = 0x1e00;
		dli_offset = 0x1e80;
	}
	else
		return FALSE;

	memcpy(color_regs, image + colors_offset, 9);
	if (mode == 1)
		color_regs[8] &= 0xf0;

	for (y = 0; y < 192; y++)
		dli_present[y] = FALSE;
	for (i = 0; i < 128; i++) {
		int y = image[dli_offset + i];
		switch (y) {
		case 0:
			break;
		case 1:
		case 2:
		case 4:
		case 5:
			return FALSE;
		default:
			if (mode == 0) {
				if (y >= 96 + 5)
					return FALSE;
				if (y == 3)
					y = 0;
				else
					y -= 5;
			}
			else {
				if (y == 100 || y == 101 || y >= 198)
					return FALSE;
				if (y == 3)
					y = 1;
				else if (y < 100)
					y -= 4;
				else {
					y -= 6;
				}
			}
			dli_present[y] = TRUE;
			break;
		}
	}

	image_info->width = 320;
	image_info->height = 192;
	switch (mode) {
	case 0:
		image_info->original_width = 160;
		image_info->original_height = 96;
		break;
	case 1:
	case 2:
	case 3:
		image_info->original_width = 80;
		image_info->original_height = 192;
		break;
	case 4:
		image_info->original_width = 160;
		image_info->original_height = 192;
		break;
	}

	for (y = 0; y < image_info->original_height; y++) {
		switch (mode) {
		case 0:
			color_regs[3] = color_regs[8];
			decode_video_memory(
				raw_image, color_regs + 3,
				40 * y, 0, y * 2, 1, 0, 40, 2, 15,
				frame);
			break;
		case 1:
			decode_video_memory(
				raw_image, color_regs + 8,
				40 * y, 40, y, 1, 0, 40, 1, 9,
				frame);
			break;
		case 2:
			decode_video_memory(
				raw_image, color_regs,
				40 * y, 40, y, 1, 0, 40, 1, 10,
				frame);
			break;
		case 3:
			decode_video_memory(
				raw_image, color_regs + 8,
				40 * y, 40, y, 1, 0, 40, 1, FAIL_MODE_REAL11,
				frame);
			break;
		case 4:
			color_regs[3] = color_regs[8];
			decode_video_memory(
				raw_image, color_regs + 3,
				40 * y, 40, y, 1, 0, 40, 1, 15,
				frame);
			break;
		}
		if (dli_present[y]) {
			int vcount = mode == 0 ? 16 + y : 16 + ((y - 1) >> 1);
			int reg = image[dli_offset + 128 + vcount];
			if (reg < 9)
				color_regs[reg] = image[dli_offset + 256 + vcount];
			else if (reg != 0x80)
				return FALSE;
		}
	}

	frame_to_rgb(frame, 320 * 192, atari_palette, pixels);
	return TRUE;
}

static abool decode_rm0(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	return decode_rm(image, image_len, atari_palette, image_info, pixels, 0);
}

static abool decode_rm1(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	return decode_rm(image, image_len, atari_palette, image_info, pixels, 1);
}

static abool decode_rm2(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	return decode_rm(image, image_len, atari_palette, image_info, pixels, 2);
}

static abool decode_rm3(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	return decode_rm(image, image_len, atari_palette, image_info, pixels, 3);
}

static abool decode_rm4(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	return decode_rm(image, image_len, atari_palette, image_info, pixels, 4);
}

static abool unpack_xlp(const byte data[], int data_len, byte unpacked_data[], int unpacked_data_len)
{
	int data_offset = 0;
	int unpacked_offset = 0;
	for (;;) {
		int b;
		int len;
		if (data_offset >= data_len)
			return FALSE;
		b = data[data_offset++];
		len = b & 0x7f;
		if (len >= 0x40) {
			if (data_offset >= data_len)
				return FALSE;
			len = ((len - 0x40) << 8) + data[data_offset++];
		}
		if (b < 0x80)
			b = -1;
		else {
			if (data_offset >= data_len)
				return FALSE;
			b = data[data_offset++];
		}
		while (len > 0) {
			if (b < 0) {
				if (data_offset >= data_len)
					return FALSE;
				unpacked_data[unpacked_offset] = data[data_offset++];
			}
			else
				unpacked_data[unpacked_offset] = (byte) b;
			unpacked_offset += 40;
			if (unpacked_offset >= unpacked_data_len) {
				unpacked_offset -= unpacked_data_len - 1;
				if (unpacked_offset >= 40)
					return TRUE;
			}
			len--;
		}
	}
}

static abool decode_xlp(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	int height;
	byte unpacked_image[2 * 8000];
	byte frame1[320 * 200];
	byte frame2[320 * 200];

	if (image_len >= 10 && image[0] == 'X' && image[1] == 'L' && image[2] == 'P' && image[3] == 'C') {
		image += 4;
		image_len -= 4;
		/* ignore errors: STAIRS.XLP included with XL-Paint is missing last two image bytes */
		memset(unpacked_image, 0, 2 * 40 * 192);
		unpack_xlp(image + 4, image_len - 4, unpacked_image, 2 * 40 * 192);
		height = 192;
	}
	else {
		height = 200;
		/* no header so better check for errors */
		if (!unpack_xlp(image + 4, image_len - 4, unpacked_image, 2 * 40 * 200))
			return FALSE;
	}

	image_info->width = 320;
	image_info->height = height;
	image_info->original_width = 160;
	image_info->original_height = height;

	decode_video_memory(
		unpacked_image, image,
		0, 40, 0, 1, 0, 40, height, FAIL_MODE_15PF0FIRST,
		frame1);

	decode_video_memory(
		unpacked_image, image,
		40 * height, 40, 0, 1, 0, 40, height, FAIL_MODE_15PF0FIRST,
		frame2);

	frames_to_rgb(frame1, frame2, 320 * height, atari_palette, pixels);
	return TRUE;
}

static abool decode_max(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	byte unpacked_image[2 * 7680];
	byte frame1[320 * 192];
	byte frame2[320 * 192];
	int y;

	if (image_len < 4 + 9 * 192 || image[0] != 'X' || image[1] != 'L' || image[2] != 'P' || image[3] != 'M'
	 || !unpack_xlp(image + 4 + 9 * 192, image_len - (4 + 9 * 192), unpacked_image, 2 * 7680))
		return FALSE;

	image_info->width = 320;
	image_info->height = 192;
	image_info->original_width = 160;
	image_info->original_height = 192;

	for (y = 0; y < 192; y++) {
		byte color_regs[4] = { image[4 + y], image[4 + 192 + y], image[4 + 2 * 192 + y], image[4 + 3 * 192 + y] };
		decode_video_memory(
			unpacked_image, color_regs,
			7680 + 40 * y, 40, y, 1, 0, 40, 1, 15,
			frame1);

		color_regs[0] = image[4 + 4 * 192 + y];
		color_regs[1] = image[4 + 5 * 192 + y];
		color_regs[2] = image[4 + 6 * 192 + y];
		color_regs[3] = image[4 + 7 * 192 + y];
		decode_video_memory(
			unpacked_image, color_regs,
			40 * y, 40, y, 1, 0, 40, 1, 15,
			frame2);
	}

	frames_to_rgb(frame1, frame2, 320 * 192, atari_palette, pixels);
	return TRUE;
}

static abool decode_shc(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	byte frame1[320 * 192];
	byte frame2[320 * 192];
	int col1 = 0x3c00;
	int col2 = 0x4100;
	int y;

	if (image_len != 17920)
		return FALSE;

	for (y = 0; y < 192; y++) {
		int x;
		for (x = 0; x < 320; x++) {
			int i = 320 * y + x;
			switch (x) {
			/* widths measured in Atari800Win PLus 4.0 and Altirra 2.0-test47 */
			case 46+48:
			case 46+48+48+24:
			case 46+48+48+24+24+24:
			case 46+48+48+24+24+24+24+24:
			case 46+48+48+24+24+24+24+24+24+20:
				col1++;
				break;
			case 46:
			case 46+48+48:
			case 46+48+48+24+24:
			case 46+48+48+24+24+24+24:
			case 46+48+48+24+24+24+24+24+24:
				col2++;
				break;
			default:
				break;
			}
			/* COLPF1==0 */
			frame1[i] = image[col1] & ((image[i >> 3] >> (~i & 7) & 1) != 0 ? 0xf0 : 0xfe);
			frame2[i] = image[col2] & ((image[7680 + (i >> 3)] >> (~i & 7) & 1) != 0 ? 0xf0 : 0xfe);
		}
		col1++;
		col2++;
	}

	image_info->original_width = image_info->width = 320;
	image_info->original_height = image_info->height = 192;
	frames_to_rgb(frame1, frame2, 320 * 192, atari_palette, pixels);
	return TRUE;
}

static abool decode_all(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	/* "ALL" format of GRAPH.COM by Adam Bienias:
	24 bytes - font in line (0-7)
	n*1024 bytes - fonts
	960 bytes - characters
	5 bytes - PF0, PF1, PF2, PF3, BAK colors */

	byte frame[320 * 192];
	int y;

	if ((image_len & 0x3ff) != 24 + 960 + 5)
		return FALSE;

	for (y = 0; y < 192; y++) {
		int font_offset = 24 + (image[y >> 3] << 10);
		int x;
		if (font_offset >= image_len - 965)
			return FALSE;
		for (x = 0; x < 160; x++) {
			int ch = image[image_len - 965 + 40 * (y >> 3) + (x >> 2)];
			int color = (image[font_offset + ((ch & 0x7f) << 3) + (y & 7)] >> (2 * (~x & 3))) & 3;
			switch (color) {
			case 0:
				color = 4;
				break;
			case 1:
				color = 0;
				break;
			case 2:
				color = 1;
				break;
			case 3:
				if (ch < 0x80)
					color = 2;
				break;
			}
			frame[320 * y + 2 * x + 1] = frame[320 * y + 2 * x] = image[image_len - 5 + color] & 0xfe;
		}
	}

	image_info->width = 320;
	image_info->height = 192;
	image_info->original_width = 160;
	image_info->original_height = 192;
	frame_to_rgb(frame, 320 * 192, atari_palette, pixels);
	return TRUE;
}

static abool decode_app(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	byte unpacked_image[15872];
	int unpacked_offset;
	int current;
	int hi;
	int image_bits;
	int image_offset;

	if (image_len < 22 + 15872 / 2 /* two or more compressed bits per uncompressed nibble */
	 || image[0] != 'S' || image[1] != '1' || image[2] != '0' || image[3] != '1'
	 || image[4] != 0 || image[5] != 0x3e)
		return FALSE;

	unpacked_offset = 0;
	current = image[22] >> 4;
	hi = -1;
	image_bits = (image[22] << 4) | 8;
	image_offset = 23;

	for (;;) {
		int code;
		if (hi < 0)
			hi = current;
		else {
			unpacked_image[unpacked_offset++] = (hi << 4) | current;
			if (unpacked_offset >= 15872)
				break;
			hi = -1;
		}
		for (code = 0; ; code += 2) {
#define APP_NEXT_BIT() \
			if ((image_bits & 0xff) != 0x80) \
				image_bits <<= 1; \
			else { \
				if (image_offset >= image_len) \
					return FALSE; \
				image_bits = (image[image_offset++] << 1) | 1; \
			}
			APP_NEXT_BIT();
			if ((image_bits & 0x100) == 0)
				break;
			if (code >= 14)
				return FALSE;
		}
		APP_NEXT_BIT();
		code += (image_bits >> 8) & 1;
		current = (current - image[6 + (code & 0xff)]) & 0xf;
#undef APP_NEXT_BIT
	}

	return decode_ap3(unpacked_image, 15872, atari_palette, image_info, pixels);
}

static const byte atari_font[] =
{
	/* space, exclamation, ... */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x00,
	0x00, 0x66, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x66, 0xff, 0x66, 0x66, 0xff, 0x66, 0x00,
	0x18, 0x3e, 0x60, 0x3c, 0x06, 0x7c, 0x18, 0x00,
	0x00, 0x66, 0x6c, 0x18, 0x30, 0x66, 0x46, 0x00,
	0x1c, 0x36, 0x1c, 0x38, 0x6f, 0x66, 0x3b, 0x00,
	0x00, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x0e, 0x1c, 0x18, 0x18, 0x1c, 0x0e, 0x00,
	0x00, 0x70, 0x38, 0x18, 0x18, 0x38, 0x70, 0x00,
	0x00, 0x66, 0x3c, 0xff, 0x3c, 0x66, 0x00, 0x00,
	0x00, 0x18, 0x18, 0x7e, 0x18, 0x18, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30,
	0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00,
	0x00, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x40, 0x00,
	0x00, 0x3c, 0x66, 0x6e, 0x76, 0x66, 0x3c, 0x00,
	0x00, 0x18, 0x38, 0x18, 0x18, 0x18, 0x7e, 0x00,
	0x00, 0x3c, 0x66, 0x0c, 0x18, 0x30, 0x7e, 0x00,
	0x00, 0x7e, 0x0c, 0x18, 0x0c, 0x66, 0x3c, 0x00,
	0x00, 0x0c, 0x1c, 0x3c, 0x6c, 0x7e, 0x0c, 0x00,
	0x00, 0x7e, 0x60, 0x7c, 0x06, 0x66, 0x3c, 0x00,
	0x00, 0x3c, 0x60, 0x7c, 0x66, 0x66, 0x3c, 0x00,
	0x00, 0x7e, 0x06, 0x0c, 0x18, 0x30, 0x30, 0x00,
	0x00, 0x3c, 0x66, 0x3c, 0x66, 0x66, 0x3c, 0x00,
	0x00, 0x3c, 0x66, 0x3e, 0x06, 0x0c, 0x38, 0x00,
	0x00, 0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00,
	0x00, 0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x30,
	0x06, 0x0c, 0x18, 0x30, 0x18, 0x0c, 0x06, 0x00,
	0x00, 0x00, 0x7e, 0x00, 0x00, 0x7e, 0x00, 0x00,
	0x60, 0x30, 0x18, 0x0c, 0x18, 0x30, 0x60, 0x00,
	0x00, 0x3c, 0x66, 0x0c, 0x18, 0x00, 0x18, 0x00,

	/* @, A, ... */
	0x00, 0x3c, 0x66, 0x6e, 0x6e, 0x60, 0x3e, 0x00,
	0x00, 0x18, 0x3c, 0x66, 0x66, 0x7e, 0x66, 0x00,
	0x00, 0x7c, 0x66, 0x7c, 0x66, 0x66, 0x7c, 0x00,
	0x00, 0x3c, 0x66, 0x60, 0x60, 0x66, 0x3c, 0x00,
	0x00, 0x78, 0x6c, 0x66, 0x66, 0x6c, 0x78, 0x00,
	0x00, 0x7e, 0x60, 0x7c, 0x60, 0x60, 0x7e, 0x00,
	0x00, 0x7e, 0x60, 0x7c, 0x60, 0x60, 0x60, 0x00,
	0x00, 0x3e, 0x60, 0x60, 0x6e, 0x66, 0x3e, 0x00,
	0x00, 0x66, 0x66, 0x7e, 0x66, 0x66, 0x66, 0x00,
	0x00, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x7e, 0x00,
	0x00, 0x06, 0x06, 0x06, 0x06, 0x66, 0x3c, 0x00,
	0x00, 0x66, 0x6c, 0x78, 0x78, 0x6c, 0x66, 0x00,
	0x00, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7e, 0x00,
	0x00, 0x63, 0x77, 0x7f, 0x6b, 0x63, 0x63, 0x00,
	0x00, 0x66, 0x76, 0x7e, 0x7e, 0x6e, 0x66, 0x00,
	0x00, 0x3c, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00,
	0x00, 0x7c, 0x66, 0x66, 0x7c, 0x60, 0x60, 0x00,
	0x00, 0x3c, 0x66, 0x66, 0x66, 0x6c, 0x36, 0x00,
	0x00, 0x7c, 0x66, 0x66, 0x7c, 0x6c, 0x66, 0x00,
	0x00, 0x3c, 0x60, 0x3c, 0x06, 0x06, 0x3c, 0x00,
	0x00, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00,
	0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x7e, 0x00,
	0x00, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x00,
	0x00, 0x63, 0x63, 0x6b, 0x7f, 0x77, 0x63, 0x00,
	0x00, 0x66, 0x66, 0x3c, 0x3c, 0x66, 0x66, 0x00,
	0x00, 0x66, 0x66, 0x3c, 0x18, 0x18, 0x18, 0x00,
	0x00, 0x7e, 0x0c, 0x18, 0x30, 0x60, 0x7e, 0x00,
	0x00, 0x1e, 0x18, 0x18, 0x18, 0x18, 0x1e, 0x00,
	0x00, 0x40, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x00,
	0x00, 0x78, 0x18, 0x18, 0x18, 0x18, 0x78, 0x00,
	0x00, 0x08, 0x1c, 0x36, 0x63, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00,

	/* heart, ... */
	0x00, 0x36, 0x7f, 0x7f, 0x3e, 0x1c, 0x08, 0x00,
	0x18, 0x18, 0x18, 0x1f, 0x1f, 0x18, 0x18, 0x18,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x18, 0x18, 0x18, 0xf8, 0xf8, 0x00, 0x00, 0x00,
	0x18, 0x18, 0x18, 0xf8, 0xf8, 0x18, 0x18, 0x18,
	0x00, 0x00, 0x00, 0xf8, 0xf8, 0x18, 0x18, 0x18,
	0x03, 0x07, 0x0e, 0x1c, 0x38, 0x70, 0xe0, 0xc0,
	0xc0, 0xe0, 0x70, 0x38, 0x1c, 0x0e, 0x07, 0x03,
	0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff,
	0x00, 0x00, 0x00, 0x00, 0x0f, 0x0f, 0x0f, 0x0f,
	0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff,
	0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00,
	0xf0, 0xf0, 0xf0, 0xf0, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00, 0xf0, 0xf0, 0xf0, 0xf0,
	0x00, 0x1c, 0x1c, 0x77, 0x77, 0x08, 0x1c, 0x00,
	0x00, 0x00, 0x00, 0x1f, 0x1f, 0x18, 0x18, 0x18,
	0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00,
	0x18, 0x18, 0x18, 0xff, 0xff, 0x18, 0x18, 0x18,
	0x00, 0x00, 0x3c, 0x7e, 0x7e, 0x7e, 0x3c, 0x00,
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
	0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
	0x00, 0x00, 0x00, 0xff, 0xff, 0x18, 0x18, 0x18,
	0x18, 0x18, 0x18, 0xff, 0xff, 0x00, 0x00, 0x00,
	0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0,
	0x18, 0x18, 0x18, 0x1f, 0x1f, 0x00, 0x00, 0x00,
	0x78, 0x60, 0x78, 0x60, 0x7e, 0x18, 0x1e, 0x00,
	0x00, 0x18, 0x3c, 0x7e, 0x18, 0x18, 0x18, 0x00,
	0x00, 0x18, 0x18, 0x18, 0x7e, 0x3c, 0x18, 0x00,
	0x00, 0x18, 0x30, 0x7e, 0x30, 0x18, 0x00, 0x00,
	0x00, 0x18, 0x0c, 0x7e, 0x0c, 0x18, 0x00, 0x00,

	/* diamond, a, ... */
	0x00, 0x18, 0x3c, 0x7e, 0x7e, 0x3c, 0x18, 0x00,
	0x00, 0x00, 0x3c, 0x06, 0x3e, 0x66, 0x3e, 0x00,
	0x00, 0x60, 0x60, 0x7c, 0x66, 0x66, 0x7c, 0x00,
	0x00, 0x00, 0x3c, 0x60, 0x60, 0x60, 0x3c, 0x00,
	0x00, 0x06, 0x06, 0x3e, 0x66, 0x66, 0x3e, 0x00,
	0x00, 0x00, 0x3c, 0x66, 0x7e, 0x60, 0x3c, 0x00,
	0x00, 0x0e, 0x18, 0x3e, 0x18, 0x18, 0x18, 0x00,
	0x00, 0x00, 0x3e, 0x66, 0x66, 0x3e, 0x06, 0x7c,
	0x00, 0x60, 0x60, 0x7c, 0x66, 0x66, 0x66, 0x00,
	0x00, 0x18, 0x00, 0x38, 0x18, 0x18, 0x3c, 0x00,
	0x00, 0x06, 0x00, 0x06, 0x06, 0x06, 0x06, 0x3c,
	0x00, 0x60, 0x60, 0x6c, 0x78, 0x6c, 0x66, 0x00,
	0x00, 0x38, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00,
	0x00, 0x00, 0x66, 0x7f, 0x7f, 0x6b, 0x63, 0x00,
	0x00, 0x00, 0x7c, 0x66, 0x66, 0x66, 0x66, 0x00,
	0x00, 0x00, 0x3c, 0x66, 0x66, 0x66, 0x3c, 0x00,
	0x00, 0x00, 0x7c, 0x66, 0x66, 0x7c, 0x60, 0x60,
	0x00, 0x00, 0x3e, 0x66, 0x66, 0x3e, 0x06, 0x06,
	0x00, 0x00, 0x7c, 0x66, 0x60, 0x60, 0x60, 0x00,
	0x00, 0x00, 0x3e, 0x60, 0x3c, 0x06, 0x7c, 0x00,
	0x00, 0x18, 0x7e, 0x18, 0x18, 0x18, 0x0e, 0x00,
	0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3e, 0x00,
	0x00, 0x00, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x00,
	0x00, 0x00, 0x63, 0x6b, 0x7f, 0x3e, 0x36, 0x00,
	0x00, 0x00, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0x00,
	0x00, 0x00, 0x66, 0x66, 0x66, 0x3e, 0x0c, 0x78,
	0x00, 0x00, 0x7e, 0x0c, 0x18, 0x30, 0x7e, 0x00,
	0x00, 0x18, 0x3c, 0x7e, 0x7e, 0x18, 0x3c, 0x00,
	0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x00, 0x7e, 0x78, 0x7c, 0x6e, 0x66, 0x06, 0x00,
	0x08, 0x18, 0x38, 0x78, 0x38, 0x18, 0x08, 0x00,
	0x10, 0x18, 0x1c, 0x1e, 0x1c, 0x18, 0x10, 0x00,

	/* SGE redefined characters */
#if 0
	/* grayed in the editor, as a warning that they're non-standard */
	0x05, 0x0a, 0x05, 0x0a, 0x50, 0xa0, 0x50, 0xa0,
	0xa0, 0x50, 0xa0, 0x50, 0x0a, 0x05, 0x0a, 0x05
#else
	/* solid */
	0x0f, 0x0f, 0x0f, 0x0f, 0xf0, 0xf0, 0xf0, 0xf0,
	0xf0, 0xf0, 0xf0, 0xf0, 0x0f, 0x0f, 0x0f, 0x0f
#endif
};

static abool decode_sge(
	const byte image[], int image_len,
	const byte atari_palette[],
	FAIL_ImageInfo* image_info,
	byte pixels[])
{
	int y;
	byte gr8[7680];

	if (image_len != 960)
		return FALSE;

	for (y = 0; y < 192; y++) {
		int x;
		for (x = 0; x < 40; x++) {
			int ch = image[40 * (y >> 3) + x];
			int inv = ch >= 0x80 ? 0xff : 0;
			ch &= 0x7f;
			switch (ch) {
			case 0x1b:
				ch = 0x80;
				break;
			case 0x7d:
				ch = 0x81;
				break;
			default:
				break;
			}
			gr8[40 * y + x] = atari_font[(ch << 3) + (y & 7)] ^ inv;
		}
	}

	return decode_gr8(gr8, 7680, atari_palette, image_info, pixels);
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
	case FAIL_EXT('H', 'R', '2'):
	case FAIL_EXT('M', 'C', 'H'):
	case FAIL_EXT('I', 'G', 'E'):
	case FAIL_EXT('2', '5', '6'):
	case FAIL_EXT('A', 'P', '2'):
	case FAIL_EXT('J', 'G', 'P'):
	case FAIL_EXT('D', 'G', 'P'):
	case FAIL_EXT('E', 'S', 'C'):
	case FAIL_EXT('P', 'Z', 'M'):
	case FAIL_EXT('I', 'S', 'T'):
	case FAIL_EXT('R', 'A', 'W'):
	case FAIL_EXT('R', 'G', 'B'):
	case FAIL_EXT('M', 'G', 'P'):
	case FAIL_EXT('W', 'N', 'D'):
	case FAIL_EXT('C', 'H', 'R'):
	case FAIL_EXT('S', 'H', 'P'):
	case FAIL_EXT('M', 'B', 'G'):
	case FAIL_EXT('F', 'W', 'A'):
	case FAIL_EXT('R', 'M', '0'):
	case FAIL_EXT('R', 'M', '1'):
	case FAIL_EXT('R', 'M', '2'):
	case FAIL_EXT('R', 'M', '3'):
	case FAIL_EXT('R', 'M', '4'):
	case FAIL_EXT('X', 'L', 'P'):
	case FAIL_EXT('M', 'A', 'X'):
	case FAIL_EXT('S', 'H', 'C'):
	case FAIL_EXT('A', 'L', 'L'):
	case FAIL_EXT('A', 'P', 'P'):
	case FAIL_EXT('S', 'G', 'E'):
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
		{ FAIL_EXT('G', 'H', 'G'), decode_ghg },
		{ FAIL_EXT('H', 'R', '2'), decode_hr2 },
		{ FAIL_EXT('M', 'C', 'H'), decode_mch },
		{ FAIL_EXT('I', 'G', 'E'), decode_ige },
		{ FAIL_EXT('2', '5', '6'), decode_256 },
		{ FAIL_EXT('A', 'P', '2'), decode_256 },
		{ FAIL_EXT('J', 'G', 'P'), decode_jgp },
		{ FAIL_EXT('D', 'G', 'P'), decode_pzm },
		{ FAIL_EXT('E', 'S', 'C'), decode_pzm },
		{ FAIL_EXT('P', 'Z', 'M'), decode_pzm },
		{ FAIL_EXT('I', 'S', 'T'), decode_ist },
		{ FAIL_EXT('R', 'A', 'W'), decode_raw },
		{ FAIL_EXT('R', 'G', 'B'), decode_rgb },
		{ FAIL_EXT('M', 'G', 'P'), decode_mgp },
		{ FAIL_EXT('W', 'N', 'D'), decode_wnd },
		{ FAIL_EXT('C', 'H', 'R'), decode_chr },
		{ FAIL_EXT('S', 'H', 'P'), decode_shp },
		{ FAIL_EXT('M', 'B', 'G'), decode_mbg },
		{ FAIL_EXT('F', 'W', 'A'), decode_fwa },
		{ FAIL_EXT('R', 'M', '0'), decode_rm0 },
		{ FAIL_EXT('R', 'M', '1'), decode_rm1 },
		{ FAIL_EXT('R', 'M', '2'), decode_rm2 },
		{ FAIL_EXT('R', 'M', '3'), decode_rm3 },
		{ FAIL_EXT('R', 'M', '4'), decode_rm4 },
		{ FAIL_EXT('X', 'L', 'P'), decode_xlp },
		{ FAIL_EXT('M', 'A', 'X'), decode_max },
		{ FAIL_EXT('S', 'H', 'C'), decode_shc },
		{ FAIL_EXT('A', 'L', 'L'), decode_all },
		{ FAIL_EXT('A', 'P', 'P'), decode_app },
		{ FAIL_EXT('S', 'G', 'E'), decode_sge }
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