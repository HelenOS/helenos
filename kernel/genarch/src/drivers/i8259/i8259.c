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
 * @brief i8259 driver.
 *
 * Programmable Interrupt Controller for UP systems based on i8259 chip.
 */

#include <genarch/drivers/i8259/i8259.h>
#include <typedefs.h>
#include <stdint.h>
#include <log.h>
#include <interrupt.h>

/* ICW1 bits */
#define I8259_ICW1           (1 << 4)
#define I8259_ICW1_NEEDICW4  (1 << 0)

/* OCW3 bits */
#define I8259_OCW3           (1 << 3)
#define I8259_OCW3_READ_ISR  (3 << 0)

/* OCW4 bits */
#define I8259_OCW4           (0 << 3)
#define I8259_OCW4_NSEOI     (1 << 5)

#define I8259_IRQ_COUNT 8

#define I8259_IRQ_SLAVE 2

static const char *i8259_get_name(void);

pic_ops_t i8259_pic_ops = {
	.get_name = i8259_get_name,
	.enable_irqs = i8259_enable_irqs,
	.disable_irqs = i8259_disable_irqs,
	.eoi = i8259_eoi,
	.is_spurious = i8259_is_spurious,
	.handle_spurious = i8259_handle_spurious
};

// XXX: need to change pic_* API to get rid of these
static i8259_t *saved_pic0;
static i8259_t *saved_pic1;

void i8259_init(i8259_t *pic0, i8259_t *pic1, unsigned int irq0_vec)
{
	saved_pic0 = pic0;
	saved_pic1 = pic1;

	/* ICW1: this is ICW1, ICW4 to follow */
	pio_write_8(&pic0->port1, I8259_ICW1 | I8259_ICW1_NEEDICW4);

	/* ICW2: IRQ 0 maps to interrupt vector address irq0_vec */
	pio_write_8(&pic0->port2, irq0_vec);

	/* ICW3: pic1 using IRQ I8259_IRQ_SLAVE */
	pio_write_8(&pic0->port2, 1 << I8259_IRQ_SLAVE);

	/* ICW4: i8086 mode */
	pio_write_8(&pic0->port2, 1);

	/* ICW1: ICW1, ICW4 to follow */
	pio_write_8(&pic1->port1, I8259_ICW1 | I8259_ICW1_NEEDICW4);

	/* ICW2: IRQ 8 maps to interrupt vector address irq0_vec + 8 */
	pio_write_8(&pic1->port2, irq0_vec + I8259_IRQ_COUNT);

	/* ICW3: pic1 is known as I8259_IRQ_SLAVE */
	pio_write_8(&pic1->port2, I8259_IRQ_SLAVE);

	/* ICW4: i8086 mode */
	pio_write_8(&pic1->port2, 1);

	/* disable all irq's */
	i8259_disable_irqs(0xffff);
	/* but enable I8259_IRQ_SLAVE */
	i8259_enable_irqs(1 << I8259_IRQ_SLAVE);
}

const char *i8259_get_name(void)
{
	return "i8259";
}

void i8259_enable_irqs(uint16_t irqmask)
{
	uint8_t x;

	if (irqmask & 0xff) {
		x = pio_read_8(&saved_pic0->port2);
		pio_write_8(&saved_pic0->port2,
		    (uint8_t) (x & (~(irqmask & 0xff))));
	}
	if (irqmask >> I8259_IRQ_COUNT) {
		x = pio_read_8(&saved_pic1->port2);
		pio_write_8(&saved_pic1->port2,
		    (uint8_t) (x & (~(irqmask >> I8259_IRQ_COUNT))));
	}
}

void i8259_disable_irqs(uint16_t irqmask)
{
	uint8_t x;

	if (irqmask & 0xff) {
		x = pio_read_8(&saved_pic0->port2);
		pio_write_8(&saved_pic0->port2,
		    (uint8_t) (x | (irqmask & 0xff)));
	}
	if (irqmask >> I8259_IRQ_COUNT) {
		x = pio_read_8(&saved_pic1->port2);
		pio_write_8(&saved_pic1->port2,
		    (uint8_t) (x | (irqmask >> I8259_IRQ_COUNT)));
	}
}

void i8259_eoi(unsigned int irq)
{
	if (irq >= I8259_IRQ_COUNT)
		pio_write_8(&saved_pic1->port1, I8259_OCW4 | I8259_OCW4_NSEOI);
	pio_write_8(&saved_pic0->port1, I8259_OCW4 | I8259_OCW4_NSEOI);
}

bool i8259_is_spurious(unsigned int irq)
{
	pio_write_8(&saved_pic0->port1, I8259_OCW3 | I8259_OCW3_READ_ISR);
	pio_write_8(&saved_pic1->port1, I8259_OCW3 | I8259_OCW3_READ_ISR);
	uint8_t isr_lo = pio_read_8(&saved_pic0->port1);
	uint8_t isr_hi = pio_read_8(&saved_pic1->port1);
	return !(((isr_hi << I8259_IRQ_COUNT) | isr_lo) & (1 << irq));
}

void i8259_handle_spurious(unsigned int irq)
{
	/* For spurious IRQs from pic1, we need to isssue an EOI to pic0 */
	if (irq >= I8259_IRQ_COUNT)
		pio_write_8(&saved_pic0->port1, I8259_OCW4 | I8259_OCW4_NSEOI);
}

/** @}
 */
