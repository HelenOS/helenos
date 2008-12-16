/*
 * Copyright (c) 2006 Ondrej Palkovsky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <errno.h>

#include "ppm.h"

static void skip_whitespace(unsigned char **data)
{
retry:
	while (**data == ' ' || **data == '\t' || **data == '\n' ||
	    **data == '\r')
		(*data)++;
	if (**data == '#') {
		while (1) {
			if (**data == '\n' || **data == '\r')
				break;
			(*data)++;
		}
		goto retry;
	}
}

static void read_num(unsigned char **data, unsigned int *num)
{
	*num = 0;
	while (**data >= '0' && **data <= '9') {
		*num *= 10;
		*num += **data - '0';
		(*data)++;
	}
}

int ppm_get_data(unsigned char *data, size_t dtsz, unsigned int *width,
    unsigned int *height)
{
	/* Read magic */
	if (data[0] != 'P' || data[1] != '6')
		return EINVAL;

	data+=2;
	skip_whitespace(&data);
	read_num(&data, width);
	skip_whitespace(&data);
	read_num(&data,height);

	return 0;
}

/** Draw PPM pixmap
 *
 * @param data Pointer to PPM data
 * @param datasz Maximum data size
 * @param sx Coordinate of upper left corner
 * @param sy Coordinate of upper left corner
 * @param maxwidth Maximum allowed width for picture
 * @param maxheight Maximum allowed height for picture
 * @param putpixel Putpixel function used to print bitmap
 */
int ppm_draw(unsigned char *data, size_t datasz, unsigned int sx,
    unsigned int sy, unsigned int maxwidth, unsigned int maxheight,
    putpixel_cb_t putpixel, void *vport)
{
	unsigned int width, height;
	unsigned int maxcolor;
	int i;
	unsigned int color;
	unsigned int coef;
	
	/* Read magic */
	if ((data[0] != 'P') || (data[1] != '6'))
		return EINVAL;
	
	data += 2;
	skip_whitespace(&data);
	read_num(&data, &width);
	skip_whitespace(&data);
	read_num(&data, &height);
	skip_whitespace(&data);
	read_num(&data, &maxcolor);
	data++;
	
	if ((maxcolor == 0) || (maxcolor > 255) || (width * height > datasz))
		return EINVAL;
	
	coef = 255 / maxcolor;
	if (coef * maxcolor > 255)
		coef -= 1;
	
	for (i = 0; i < width * height; i++) {
		/* Crop picture if we don't fit into region */
		if (i % width > maxwidth || i / width > maxheight) {
			data += 3;
			continue;
		}
		color = ((data[0] * coef) << 16) + ((data[1] * coef) << 8) +
		    data[2] * coef;
		
		(*putpixel)(vport, sx + (i % width), sy + (i / width), color);
		data += 3;
	}
	
	return 0;
}
