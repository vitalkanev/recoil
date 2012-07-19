/*
 * fail2png.c - command-line converter of Atari pictures to the PNG format
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "fail.h"
#include "pngsave.h"

static const char *output_file = NULL;
/* atari_palette is one byte bigger than necessary in order to check
   that the file we read is exactly FAIL_PALETTE_MAX bytes long. */
static byte atari_palette[FAIL_PALETTE_MAX + 1];
static abool use_atari_palette = FALSE;

static void print_help(void)
{
	printf(
		"Usage: fail2png [OPTIONS] INPUTFILE...\n"
		"Options:\n"
		"-o FILE  --output=FILE   Set output file name\n"
		"-p FILE  --palette=FILE  Load Atari 8-bit palette (768 bytes)\n"
		"-h       --help          Display this information\n"
		"-v       --version       Display version information\n"
	);
}

static void fatal_error(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	fprintf(stderr, "fail2png: ");
	vfprintf(stderr, format, args);
	fputc('\n', stderr);
	va_end(args);
	exit(1);
}

static int load_file(const char *filename, byte buffer[], size_t buffer_len)
{
	FILE *fp;
	int len;
	fp = fopen(filename, "rb");
	if (fp == NULL)
		fatal_error("cannot open %s", filename);
	len = fread(buffer, 1, buffer_len, fp);
	fclose(fp);
	return len;
}

static void load_palette(const char *filename)
{
	if (load_file(filename, atari_palette, sizeof(atari_palette)) != FAIL_PALETTE_MAX)
		fatal_error("%s: palette file must be 768 bytes", filename);
	use_atari_palette = TRUE;
}

static void process_file(const char *input_file)
{
	static byte image[FAIL_IMAGE_MAX];
	int image_len;
	FAIL_ImageInfo image_info;
	static byte pixels[FAIL_PIXELS_MAX];
	static byte palette[FAIL_PALETTE_MAX];
	image_len = load_file(input_file, image, sizeof(image));
	if (!FAIL_DecodeImage(input_file, image, image_len,
		use_atari_palette ? atari_palette : NULL,
		&image_info, pixels, palette)) {
		fatal_error("%s: file decoding error", input_file);
	}
	if (output_file == NULL) {
		static char output_default[FILENAME_MAX];
		int i;
		int dotp = 0;
		for (i = 0; input_file[i] != '\0' && i < FILENAME_MAX - 5; i++)
			if ((output_default[i] = input_file[i]) == '.')
				dotp = i;
		strcpy(output_default + (dotp == 0 ? i : dotp), ".png");
		output_file = output_default;
	}
	if (!PNG_Save(output_file, image_info.width, image_info.height,
		image_info.colors, pixels, palette))
		fatal_error("cannot write %s", output_file);
	output_file = NULL;
}

int main(int argc, char *argv[])
{
	abool no_input_files = TRUE;
	int i;
	for (i = 1; i < argc; i++) {
		const char *arg = argv[i];
		if (arg[0] != '-') {
			process_file(arg);
			no_input_files = FALSE;
		}
		else if (arg[1] == 'o' && arg[2] == '\0')
			output_file = argv[++i];
		else if (strncmp(arg, "--output=", 9) == 0)
			output_file = arg + 9;
		else if (arg[1] == 'p' && arg[2] == '\0')
			load_palette(argv[++i]);
		else if (strncmp(arg, "--palette=", 10) == 0)
			load_palette(arg + 10);
		else if ((arg[1] == 'h' && arg[2] == '\0')
			|| strcmp(arg, "--help") == 0) {
			print_help();
			no_input_files = FALSE;
		}
		else if ((arg[1] == 'v' && arg[2] == '\0')
			|| strcmp(arg, "--version") == 0) {
			printf("fail2png " FAIL_VERSION "\n");
			no_input_files = FALSE;
		}
		else
			fatal_error("unknown option: %s", arg);
	}
	if (no_input_files) {
		print_help();
		return 1;
	}
	return 0;
}
