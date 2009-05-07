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

#include "fail.h"

#include "palette.h"

abool FAIL_IsOurFile(const char *filename)
{
	// TODO
	return FALSE;
}

abool FAIL_DecodeImage(const char *filename,
                       const byte image[], int image_len,
                       const byte atari_palette[],
                       int *width, int *height, int *colors,
                       byte pixels[], byte palette[])
{
	// TODO
	return FALSE;
}
