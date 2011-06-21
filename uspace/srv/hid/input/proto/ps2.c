/*
 * Copyright (c) 2011 Martin Decky
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

/** @addtogroup mouse_proto
 * @ingroup input
 * @{
 */
/**
 * @file
 * @brief PS/2 protocol driver.
 */

#include <mouse.h>
#include <mouse_port.h>
#include <mouse_proto.h>

#define PS2_MOUSE_OUT_INIT  0xf4
#define PS2_MOUSE_ACK       0xfa

#define BUFSIZE 3

typedef struct {
	union {
		unsigned char data[BUFSIZE];
		struct {
			unsigned int leftbtn : 1;
			unsigned int rightbtn : 1;
			unsigned int middlebtn : 1;
			unsigned int isone : 1; /* Always one */
			unsigned int xsign : 1;
			unsigned int ysign : 1;
			unsigned int xovfl : 1;
			unsigned int yovfl : 1;
			unsigned char x;
			unsigned char y;
		} val;
	} u;
} ps2packet_t;

static ps2packet_t buf;
static unsigned int bufpos;
static unsigned int leftbtn;
static unsigned int rightbtn;
static unsigned int middlebtn;

static mouse_dev_t *mouse_dev;

static int ps2_proto_init(mouse_dev_t *mdev)
{
	mouse_dev = mdev;
	bufpos = 0;
	leftbtn = 0;
	rightbtn = 0;
	
	mouse_dev->port_ops->write(PS2_MOUSE_OUT_INIT);
	return 0;
}

/** Convert 9-bit 2-complement signed number to integer */
static int bit9toint(int sign, unsigned char data)
{
	int tmp;
	
	if (!sign)
		return data;
	
	tmp = ((unsigned char) ~data) + 1;
	return -tmp;
}

/** Process mouse data */
static void ps2_proto_parse(sysarg_t data)
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
			mouse_push_event_button(mouse_dev, 1, leftbtn);
		}
		
		if (buf.u.val.rightbtn ^ rightbtn) {
			rightbtn = buf.u.val.rightbtn;
			mouse_push_event_button(mouse_dev, 2, rightbtn);
		}
		
		if (buf.u.val.middlebtn ^ middlebtn) {
			middlebtn = buf.u.val.middlebtn;
			mouse_push_event_button(mouse_dev, 3, middlebtn);
		}
		
		x = bit9toint(buf.u.val.xsign, buf.u.val.x);
		y = -bit9toint(buf.u.val.ysign, buf.u.val.y);
		
		if (x != 0 || y != 0)
			mouse_push_event_move(mouse_dev, x, y);
	}
}

mouse_proto_ops_t ps2_proto = {
	.parse = ps2_proto_parse,
	.init = ps2_proto_init
};

/**
 * @}
 */
