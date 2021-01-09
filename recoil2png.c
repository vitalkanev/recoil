/*
 * recoil2png.c - command-line converter of retro computer pictures to the PNG format
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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "recoil-stdio.h"
#include "pngsave.h"

static void print_help(void)
{
	printf(
		"Usage: recoil2png [OPTIONS] INPUTFILE...\n"
		"Options:\n"
		"-o FILE  --output=FILE   Set output file name\n"
		"         --pal           Emulate PAL video standard if applicable (default)\n"
		"         --ntsc          Emulate NTSC video standard if applicable\n"
		"-p FILE  --palette=FILE  Load Atari 8-bit palette (768 bytes)\n"
		"-h       --help          Display this information\n"
		"-v       --version       Display version information\n"
	);
}

static int load_file(const char *filename, void *buffer, size_t buffer_len)
{
	FILE *fp = fopen(filename, "rb");
	if (fp == NULL) {
		fprintf(stderr, "recoil2png: cannot open %s\n", filename);
		return -1;
	}
	int len = fread(buffer, 1, buffer_len, fp);
	fclose(fp);
	return len;
}

static bool load_palette(RECOIL *recoil, const char *filename)
{
	uint8_t atari8_palette[768 + 1];
	switch (load_file(filename, atari8_palette, sizeof(atari8_palette))) {
	case 768:
		RECOIL_SetAtari8Palette(recoil, atari8_palette);
		return true;
	case -1:
		/* error already printed */
		return false;
	default:
		fprintf(stderr, "recoil2png: %s: palette file must be 768 bytes long\n", filename);
		return false;
	}
}

static bool process_file(RECOIL *recoil, const char *input_file, const char *output_file)
{
	static uint8_t content[RECOIL_MAX_CONTENT_LENGTH];
	int content_len = load_file(input_file, content, sizeof(content));
	if (content_len < 0) {
		/* error already printed */
		return false;
	}
	if (!RECOIL_Decode(recoil, input_file, content, content_len)) {
		fprintf(stderr, "recoil2png: %s: file decoding error\n", input_file);
		return false;
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
	FILE *fp = fopen(output_file, "wb");
	if (fp == NULL || !RECOIL_SavePng(recoil, fp)) {
		fprintf(stderr, "recoil2png: cannot write %s\n", output_file);
		return false;
	}
	return true;
}

int main(int argc, char **argv)
{
	RECOIL *recoil = RECOILStdio_New();
	if (recoil == NULL) {
		fprintf(stderr, "recoil2png: out of memory\n");
		return 1;
	}
	const char *output_file = NULL;
	bool ok = true;
	bool no_input_files = true;
	for (int i = 1; i < argc; i++) {
		const char *arg = argv[i];
		if (arg[0] != '-') {
			ok &= process_file(recoil, arg, output_file);
			no_input_files = false;
			output_file = NULL;
		}
		else if (arg[1] == 'o' && arg[2] == '\0' && i + 1 < argc)
			output_file = argv[++i];
		else if (strncmp(arg, "--output=", 9) == 0)
			output_file = arg + 9;
		else if (strcmp(arg, "--pal") == 0)
			RECOIL_SetNtsc(recoil, false);
		else if (strcmp(arg, "--ntsc") == 0)
			RECOIL_SetNtsc(recoil, true);
		else if (arg[1] == 'p' && arg[2] == '\0' && i + 1 < argc) {
			if (!load_palette(recoil, argv[++i]))
				return 1;
		}
		else if (strncmp(arg, "--palette=", 10) == 0) {
			if (!load_palette(recoil, arg + 10))
				return 1;
		}
		else if ((arg[1] == 'h' && arg[2] == '\0')
			|| strcmp(arg, "--help") == 0) {
			print_help();
			no_input_files = false;
		}
		else if ((arg[1] == 'v' && arg[2] == '\0')
			|| strcmp(arg, "--version") == 0) {
			printf("recoil2png " RECOIL_VERSION "\n");
			no_input_files = false;
		}
		else {
			fprintf(stderr, "recoil2png: unknown option: %s\n", arg);
			return 1;
		}
	}
	if (no_input_files) {
		print_help();
		return 1;
	}
	return ok ? 0 : 1;
}
