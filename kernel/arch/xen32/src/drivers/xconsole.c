/*
 * Copyright (C) 2006 Martin Decky
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

/** @addtogroup xen32
 * @{
 */
/**
 * @file
 * @brief Xen32 console driver.
 */

#include <arch/drivers/xconsole.h>
#include <putchar.h>
#include <console/chardev.h>
#include <console/console.h>
#include <arch/hypercall.h>
#include <mm/frame.h>

#define MASK_INDEX(index, ring) ((index) & (sizeof(ring) - 1))

typedef struct {
	char in[1024];
	char out[2048];
    uint32_t in_cons;
	uint32_t in_prod;
    uint32_t out_cons;
	uint32_t out_prod;
} xencons_t;

static bool asynchronous = false;
static void xen_putchar(chardev_t *d, const char ch);

chardev_t xen_console;
static chardev_operations_t xen_ops = {
	.write = xen_putchar
};

void xen_console_init(void)
{
	chardev_initialize("xen_out", &xen_console, &xen_ops);
	stdout = &xen_console;
	if (!(start_info.flags & SIF_INITDOMAIN))
		asynchronous = true;
}

void xen_putchar(chardev_t *d, const char ch)
{
	if (asynchronous) {
		xencons_t *console = (xencons_t *) PA2KA(MA2PA(PFN2ADDR(start_info.console_mfn)));
		uint32_t cons = console->out_cons;
		uint32_t prod = console->out_prod;
		
		memory_barrier();
		
		if ((prod - cons) > sizeof(console->out))
			return;
		
		if (ch == '\n')
			console->out[MASK_INDEX(prod++, console->out)] = '\r';
		console->out[MASK_INDEX(prod++, console->out)] = ch;
		
		write_barrier();
		
		console->out_prod = prod;
		
		xen_notify_remote(start_info.console_evtchn);
	} else
		xen_console_io(CONSOLE_IO_WRITE, 1, &ch);
}

/** @}
 */
