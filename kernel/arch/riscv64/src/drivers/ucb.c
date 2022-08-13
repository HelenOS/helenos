/*
 * SPDX-FileCopyrightText: 2017 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
