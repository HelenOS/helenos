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

/** @addtogroup mouse
 * @{
 */
/**
 * @file
 * @brief PS/2 mouse protocol driver.
 */

#include <stdio.h>
#include <mouse_proto.h>
#include <c_mouse.h>

#define BUFSIZE 3

typedef struct {
	union {
		unsigned char data[BUFSIZE];
		struct {
			unsigned leftbtn : 1;
			unsigned rightbtn : 1;
			unsigned middlebtn : 1;
			unsigned isone : 1; /* Always one */
			unsigned xsign : 1;
			unsigned ysign : 1;
			unsigned xovfl : 1;
			unsigned yovfl : 1;
			unsigned char x;
			unsigned char y;
		} val;
	} u;
} ps2packet_t;

static ps2packet_t buf;
static int bufpos = 0;
static int leftbtn = 0;
static int rightbtn = 0;
static int middlebtn = 0;

int mouse_proto_init(void)
{
	return 0;
}

/** Convert 9-bit 2-complement signed number to integer */
static int bit9toint(int sign, unsigned char data)
{
	int tmp;

	if (!sign)
		return data;

	tmp = ((unsigned char)~data) + 1;
	return -tmp;
}

/** Process mouse data */
void mouse_proto_parse_byte(int data)
{
	int x, y;

	/* Check that we have not lost synchronization */
	if (bufpos == 0 && !(data & 0x8))
		return; /* Synchro lost, ignore byte */

	buf.u.data[bufpos++] = data;
	if (bufpos == BUFSIZE) {
		bufpos = 0;

		if (buf.u.val.leftbtn ^ leftbtn) {
			leftbtn = buf.u.val.leftbtn;
			mouse_ev_btn(1, leftbtn);
		}

		if (buf.u.val.rightbtn ^ rightbtn) {
			rightbtn = buf.u.val.rightbtn;
			mouse_ev_btn(2, rightbtn);
		}

		if (buf.u.val.middlebtn ^ middlebtn) {
			middlebtn = buf.u.val.middlebtn;
			mouse_ev_btn(3, middlebtn);
		}

		x =   bit9toint(buf.u.val.xsign, buf.u.val.x);
		y = - bit9toint(buf.u.val.ysign, buf.u.val.y);

		if (x != 0 || y != 0) {
			mouse_ev_move(x, y);
		}
	}

	return;
}

/**
 * @}
 */
