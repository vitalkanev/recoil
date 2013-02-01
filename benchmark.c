/*
 * benchmark.c - FAIL benchmark
 *
 * Copyright (C) 2013  Piotr Fusik and Adrian Matoga
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
#include <string.h>
#include <time.h>

#include "fail.h"

int main(int argc, char **argv)
{
	FAIL *fail;
	int i;
	if (argc < 2) {
		fprintf(stderr, "benchmark: no input files\n");
		return 1;
	}
	fail = FAIL_New();
	for (i = 1; i < argc; i++) {
		const char *arg = argv[i];
		FILE *fp;
		unsigned char content[FAIL_MAX_CONTENT_LENGTH];
		int content_len;
		unsigned char indexes[FAIL_MAX_PIXELS_LENGTH];
		int colors;
		clock_t start_time;
		clock_t decode_time;
		clock_t colors_time;
		clock_t palette_time;
		clock_t decode2_time;
		clock_t palette2_time;
		clock_t colors2_time;

		if (strcmp(arg, "--help") == 0) {
			printf("Usage: benchmark FILE...\n");
			return 0;
		}

		fp = fopen(arg, "rb");
		if (fp == NULL) {
			fprintf(stderr, "benchmark: cannot open %s\n", arg);
			return 1;
		}
		content_len = fread(content, 1, sizeof(content), fp);
		fclose(fp);

		start_time = clock();
		if (!FAIL_Decode(fail, arg, content, content_len)) {
			fprintf(stderr, "benchmark: error decoding %s\n", arg);
			return 1;
		}
		decode_time = clock();
		colors = FAIL_GetColors(fail);
		colors_time = clock();
		FAIL_ToPalette(fail, indexes);
		palette_time = clock();

		FAIL_Decode(fail, arg, content, content_len);
		decode2_time = clock();
		FAIL_ToPalette(fail, indexes);
		palette2_time = clock();
		if (FAIL_GetColors(fail) != colors) {
			fprintf(stderr, "benchmark: FAIL_GetColors failed for %s\n", arg);
			return 1;
		}
		colors2_time = clock();

		printf("%3dx%3d %4d colors Decode=%2ld,%2ld GetColors=%2ld,%2ld ToPalette=%2ld,%2ld %s\n",
			FAIL_GetWidth(fail), FAIL_GetHeight(fail), colors,
			(  decode_time -    start_time) * 1000L / CLOCKS_PER_SEC, /* FAIL_Decode time */
			( decode2_time -  palette_time) * 1000L / CLOCKS_PER_SEC, /* ditto, should be same */
			(  colors_time -   decode_time) * 1000L / CLOCKS_PER_SEC, /* FAIL_GetColors time */
			( colors2_time - palette2_time) * 1000L / CLOCKS_PER_SEC, /* FAIL_GetColors after FAIL_Palette, should be smaller */
			(palette2_time -  decode2_time) * 1000L / CLOCKS_PER_SEC, /* FAIL_GetPalette time */
			( palette_time -   colors_time) * 1000L / CLOCKS_PER_SEC, /* FAIL_GetPalette after FAIL_GetColors, should be smaller */
			arg);
	}
	return 0;
}
