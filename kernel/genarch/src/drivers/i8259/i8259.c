/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief PIC driver.
 *
 * Programmable Interrupt Controller for UP systems based on i8259 chip.
 */

#include <genarch/drivers/i8259/i8259.h>
#include <typedefs.h>
#include <stdint.h>
#include <log.h>
#include <interrupt.h>

static void pic_spurious(unsigned int n, istate_t *istate);

// XXX: need to change pic_* API to get rid of these
static i8259_t *saved_pic0;
static i8259_t *saved_pic1;

void i8259_init(i8259_t *pic0, i8259_t *pic1, inr_t pic1_irq,
    unsigned int irq0_int, unsigned int irq8_int)
{
	saved_pic0 = pic0;
	saved_pic1 = pic1;

	/* ICW1: this is ICW1, ICW4 to follow */
	pio_write_8(&pic0->port1, PIC_ICW1 | PIC_ICW1_NEEDICW4);

	/* ICW2: IRQ 0 maps to INT irq0_int */
	pio_write_8(&pic0->port2, irq0_int);

	/* ICW3: pic1 using IRQ IRQ_PIC1 */
	pio_write_8(&pic0->port2, 1 << pic1_irq);

	/* ICW4: i8086 mode */
	pio_write_8(&pic0->port2, 1);

	/* ICW1: ICW1, ICW4 to follow */
	pio_write_8(&pic1->port1, PIC_ICW1 | PIC_ICW1_NEEDICW4);

	/* ICW2: IRQ 8 maps to INT irq8_int */
	pio_write_8(&pic1->port2, irq8_int);

	/* ICW3: pic1 is known as IRQ_PIC1 */
	pio_write_8(&pic1->port2, pic1_irq);

	/* ICW4: i8086 mode */
	pio_write_8(&pic1->port2, 1);

	/*
	 * Register interrupt handler for the PIC spurious interrupt.
	 *
	 * XXX: This is currently broken. Both IRQ 7 and IRQ 15 can be spurious
	 *      or can be actual interrupts. This needs to be detected when
	 *      the interrupt happens by inspecting ISR.
	 */
	exc_register(irq0_int + 7, "pic_spurious", false,
	    (iroutine_t) pic_spurious);

	pic_disable_irqs(0xffff);		/* disable all irq's */
	pic_enable_irqs(1 << pic1_irq);		/* but enable pic1_irq */
}

void pic_enable_irqs(uint16_t irqmask)
{
	uint8_t x;

	if (irqmask & 0xff) {
		x = pio_read_8(&saved_pic0->port2);
		pio_write_8(&saved_pic0->port2,
		    (uint8_t) (x & (~(irqmask & 0xff))));
	}
	if (irqmask >> 8) {
		x = pio_read_8(&saved_pic1->port2);
		pio_write_8(&saved_pic1->port2,
		    (uint8_t) (x & (~(irqmask >> 8))));
	}
}

void pic_disable_irqs(uint16_t irqmask)
{
	uint8_t x;

	if (irqmask & 0xff) {
		x = pio_read_8(&saved_pic0->port2);
		pio_write_8(&saved_pic0->port2,
		    (uint8_t) (x | (irqmask & 0xff)));
	}
	if (irqmask >> 8) {
		x = pio_read_8(&saved_pic1->port2);
		pio_write_8(&saved_pic1->port2, (uint8_t) (x | (irqmask >> 8)));
	}
}

void pic_eoi(void)
{
	pio_write_8(&saved_pic0->port1, PIC_OCW4 | PIC_OCW4_NSEOI);
	pio_write_8(&saved_pic1->port1, PIC_OCW4 | PIC_OCW4_NSEOI);
}

void pic_spurious(unsigned int n __attribute__((unused)), istate_t *istate __attribute__((unused)))
{
#ifdef CONFIG_DEBUG
	log(LF_ARCH, LVL_DEBUG, "cpu%u: PIC spurious interrupt", CPU->id);
#endif
}

/** @}
 */
