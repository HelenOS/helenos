/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch/ucb.h>
#include <arch/arch.h>
#include <macros.h>

volatile uint64_t tohost __attribute__((section(".htif")));
volatile uint64_t fromhost __attribute__((section(".htif")));

static void poll_fromhost()
{
	uint64_t val = fromhost;
	if (!val)
		return;

	fromhost = 0;
}

void htif_cmd(uint8_t device, uint8_t cmd, uint64_t payload)
{
	uint64_t val = (((uint64_t) device) << 56) |
	    (((uint64_t) cmd) << 48) |
	    (payload & UINT64_C(0xffffffffffff));

	while (tohost)
		poll_fromhost();

	tohost = val;
}
