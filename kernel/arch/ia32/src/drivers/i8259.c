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

/** @addtogroup ia32
 * @{
 */
/**
 * @file
 * @brief PIC driver.
 *
 * Programmable Interrupt Controller for UP systems based on i8259 chip.
 */

#include <arch/drivers/i8259.h>
#include <cpu.h>
#include <stdint.h>
#include <arch/asm.h>
#include <arch.h>
#include <log.h>
#include <interrupt.h>

static void pic_spurious(unsigned int n, istate_t *istate);

void i8259_init(void)
{
	/* ICW1: this is ICW1, ICW4 to follow */
	pio_write_8(PIC_PIC0PORT1, PIC_ICW1 | PIC_ICW1_NEEDICW4);

	/* ICW2: IRQ 0 maps to INT IRQBASE */
	pio_write_8(PIC_PIC0PORT2, IVT_IRQBASE);

	/* ICW3: pic1 using IRQ IRQ_PIC1 */
	pio_write_8(PIC_PIC0PORT2, 1 << IRQ_PIC1);

	/* ICW4: i8086 mode */
	pio_write_8(PIC_PIC0PORT2, 1);

	/* ICW1: ICW1, ICW4 to follow */
	pio_write_8(PIC_PIC1PORT1, PIC_ICW1 | PIC_ICW1_NEEDICW4);

	/* ICW2: IRQ 8 maps to INT (IVT_IRQBASE + 8) */
	pio_write_8(PIC_PIC1PORT2, IVT_IRQBASE + 8);

	/* ICW3: pic1 is known as IRQ_PIC1 */
	pio_write_8(PIC_PIC1PORT2, IRQ_PIC1);

	/* ICW4: i8086 mode */
	pio_write_8(PIC_PIC1PORT2, 1);

	/*
	 * Register interrupt handler for the PIC spurious interrupt.
	 */
	exc_register(VECTOR_PIC_SPUR, "pic_spurious", false,
	    (iroutine_t) pic_spurious);

	/*
	 * Set the enable/disable IRQs handlers.
	 * Set the End-of-Interrupt handler.
	 */
	enable_irqs_function = pic_enable_irqs;
	disable_irqs_function = pic_disable_irqs;
	eoi_function = pic_eoi;
	irqs_info = "i8259";

	pic_disable_irqs(0xffff);		/* disable all irq's */
	pic_enable_irqs(1 << IRQ_PIC1);		/* but enable pic1 */
}

void pic_enable_irqs(uint16_t irqmask)
{
	uint8_t x;

	if (irqmask & 0xff) {
		x = pio_read_8(PIC_PIC0PORT2);
		pio_write_8(PIC_PIC0PORT2, (uint8_t) (x & (~(irqmask & 0xff))));
	}
	if (irqmask >> 8) {
		x = pio_read_8(PIC_PIC1PORT2);
		pio_write_8(PIC_PIC1PORT2, (uint8_t) (x & (~(irqmask >> 8))));
	}
}

void pic_disable_irqs(uint16_t irqmask)
{
	uint8_t x;

	if (irqmask & 0xff) {
		x = pio_read_8(PIC_PIC0PORT2);
		pio_write_8(PIC_PIC0PORT2, (uint8_t) (x | (irqmask & 0xff)));
	}
	if (irqmask >> 8) {
		x = pio_read_8(PIC_PIC1PORT2);
		pio_write_8(PIC_PIC1PORT2, (uint8_t) (x | (irqmask >> 8)));
	}
}

void pic_eoi(void)
{
	pio_write_8(PIC_PIC0PORT1, PIC_OCW4 | PIC_OCW4_NSEOI);
	pio_write_8(PIC_PIC1PORT1, PIC_OCW4 | PIC_OCW4_NSEOI);
}

void pic_spurious(unsigned int n __attribute__((unused)), istate_t *istate __attribute__((unused)))
{
#ifdef CONFIG_DEBUG
	log(LF_ARCH, LVL_DEBUG, "cpu%u: PIC spurious interrupt", CPU->id);
#endif
}

/** @}
 */
