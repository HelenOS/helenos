/*
 * Copyright (c) 2009 Martin Decky
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

/** @addtogroup genarch
 * @{
 */
/**
 * @file
 * @brief Dummy serial line output.
 */


#include <genarch/drivers/dsrln/dsrlnout.h>
#include <console/chardev.h>
#include <arch/asm.h>
#include <console/console.h>
#include <sysinfo/sysinfo.h>

static ioport8_t *dsrlnout_base;

static void dsrlnout_putchar(outdev_t *dev __attribute__((unused)), const char ch, bool silent)
{
	if (!silent)
		pio_write_8(dsrlnout_base, ch);
}

static outdev_t dsrlnout_console;
static outdev_operations_t dsrlnout_ops = {
	.write = dsrlnout_putchar
};

void dsrlnout_init(ioport8_t *base)
{
	/* Initialize the software structure. */
	dsrlnout_base = base;
	
	outdev_initialize("dsrlnout", &dsrlnout_console, &dsrlnout_ops);
	stdout = &dsrlnout_console;
	
	sysinfo_set_item_val("fb", NULL, true);
	sysinfo_set_item_val("fb.kind", NULL, 3);
	sysinfo_set_item_val("fb.address.physical", NULL, KA2PA(base));
}

/** @}
 */
