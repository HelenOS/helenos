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

/** @addtogroup kernel_ppc32
 * @{
 */
/** @file
 */

#include <arch/drivers/pic.h>
#include <mm/km.h>
#include <byteorder.h>
#include <bitops.h>
#include <typedefs.h>

static ioport32_t *pic = NULL;

void pic_init(uintptr_t base, size_t size, cir_t *cir, void **cir_arg)
{
	pic = (uint32_t *) km_map(base, size, KM_NATURAL_ALIGNMENT,
	    PAGE_WRITE | PAGE_NOT_CACHEABLE);
	*cir = pic_ack_interrupt;
	*cir_arg = NULL;
}

void pic_enable_interrupt(inr_t intnum)
{
	if (!pic)
		return;

	if (intnum < 32) {
		pio_write_32(&pic[PIC_MASK_LOW],
		    pio_read_32(&pic[PIC_MASK_LOW]) | (1 << intnum));
	} else {
		pio_write_32(&pic[PIC_MASK_HIGH],
		    pio_read_32(&pic[PIC_MASK_HIGH]) | (1 << (intnum - 32)));
	}
}

void pic_disable_interrupt(inr_t intnum)
{
	if (!pic)
		return;

	if (intnum < 32) {
		pio_write_32(&pic[PIC_MASK_LOW],
		    pio_read_32(&pic[PIC_MASK_LOW]) & (~(1 << intnum)));
	} else {
		pio_write_32(&pic[PIC_MASK_HIGH],
		    pio_read_32(&pic[PIC_MASK_HIGH]) & (~(1 << (intnum - 32))));
	}
}

void pic_ack_interrupt(void *arg, inr_t intnum)
{
	if (!pic)
		return;

	if (intnum < 32) {
		pio_write_32(&pic[PIC_ACK_LOW], 1 << intnum);
	} else {
		pio_write_32(&pic[PIC_ACK_HIGH], 1 << (intnum - 32));
	}
}

/** Return number of pending interrupts
 *
 */
uint8_t pic_get_pending(void)
{
	if (pic) {
		uint32_t pending;

		pending = pio_read_32(&pic[PIC_PENDING_LOW]);
		if (pending != 0)
			return fnzb32(pending);

		pending = pio_read_32(&pic[PIC_PENDING_HIGH]);
		if (pending != 0)
			return fnzb32(pending) + 32;
	}

	return 255;
}

/** @}
 */
