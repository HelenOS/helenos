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
 * @brief ADB protocol driver.
 */

#include <stdbool.h>
#include "../mouse.h"
#include "../mouse_port.h"
#include "../mouse_proto.h"

static mouse_dev_t *mouse_dev;
static bool b1_pressed;
static bool b2_pressed;

static int adb_proto_init(mouse_dev_t *mdev)
{
	mouse_dev = mdev;
	b1_pressed = false;
	b2_pressed = false;
	
	return 0;
}

/** Process mouse data */
static void adb_proto_parse(sysarg_t data)
{
	bool b1, b2;
	uint16_t udx, udy;
	int dx, dy;
	
	/* Extract fields. */
	b1 = ((data >> 15) & 1) == 0;
	udy = (data >> 8) & 0x7f;
	b2 = ((data >> 7) & 1) == 0;
	udx = data & 0x7f;
	
	/* Decode 7-bit two's complement signed values. */
	dx = (udx & 0x40) ? (udx - 0x80) : udx;
	dy = (udy & 0x40) ? (udy - 0x80) : udy;
	
	if (b1 != b1_pressed) {
		mouse_push_event_button(mouse_dev, 1, b1);
		b1_pressed = b1;
	}
	
	if (b2 != b2_pressed) {
		mouse_push_event_button(mouse_dev, 2, b2);
		b1_pressed = b1;
	}
	
	if (dx != 0 || dy != 0)
		mouse_push_event_move(mouse_dev, dx, dy, 0);
}

mouse_proto_ops_t adb_proto = {
	.parse = adb_proto_parse,
	.init = adb_proto_init
};

/**
 * @}
 */
