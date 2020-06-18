/*
 * Copyright (c) 2017 Martin Decky
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

#include <arch/drivers/ucb.h>
#include <stddef.h>
#include <stdint.h>
#include <console/chardev.h>
#include <str.h>
#include <stdlib.h>

#define HTIF_DEVICE_CONSOLE  1

#define HTIF_CONSOLE_PUTC  1

static volatile uint64_t *tohost;
static volatile uint64_t *fromhost;

static outdev_operations_t htifdev_ops = {
	.write = htif_putuchar,
	.redraw = NULL,
	.scroll_up = NULL,
	.scroll_down = NULL
};

static void poll_fromhost()
{
	uint64_t val = *fromhost;
	if (!val)
		return;

	*fromhost = 0;
}

void htif_init(volatile uint64_t *tohost_addr, volatile uint64_t *fromhost_addr)
{
	tohost = tohost_addr;
	fromhost = fromhost_addr;
}

outdev_t *htifout_init(void)
{
	outdev_t *htifdev = malloc(sizeof(outdev_t));
	if (!htifdev)
		return NULL;

	outdev_initialize("htifdev", htifdev, &htifdev_ops);
	return htifdev;
}

static void htif_cmd(uint8_t device, uint8_t cmd, uint64_t payload)
{
	uint64_t val = (((uint64_t) device) << 56) |
	    (((uint64_t) cmd) << 48) |
	    (payload & UINT64_C(0xffffffffffff));

	while (*tohost)
		poll_fromhost();

	*tohost = val;
}

void htif_putuchar(outdev_t *dev, const char32_t ch)
{
	if (ascii_check(ch))
		htif_cmd(HTIF_DEVICE_CONSOLE, HTIF_CONSOLE_PUTC, ch);
	else
		htif_cmd(HTIF_DEVICE_CONSOLE, HTIF_CONSOLE_PUTC, U_SPECIAL);
}
