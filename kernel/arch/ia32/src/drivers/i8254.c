/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32
 * @{
 */
/**
 * @file
 * @brief i8254 chip driver.
 *
 * Low level time functions.
 */

#include <stdint.h>
#include <time/clock.h>
#include <time/delay.h>
#include <arch/cycle.h>
#include <arch/interrupt.h>
#include <genarch/drivers/i8259/i8259.h>
#include <arch/drivers/i8254.h>
#include <cpu.h>
#include <config.h>
#include <arch/pm.h>
#include <arch/asm.h>
#include <arch/cpuid.h>
#include <arch.h>
#include <ddi/irq.h>

#define CLK_PORT1  ((ioport8_t *) 0x40U)
#define CLK_PORT4  ((ioport8_t *) 0x43U)

#define CLK_CONST     1193180
#define MAGIC_NUMBER  1194

#define LOOPS  150000
#define SHIFT  11

static irq_t i8254_irq;

static irq_ownership_t i8254_claim(irq_t *irq)
{
	return IRQ_ACCEPT;
}

static void i8254_irq_handler(irq_t *irq)
{
	/*
	 * This IRQ is responsible for kernel preemption.
	 * Nevertheless, we are now holding a spinlock which prevents
	 * preemption. For this particular IRQ, we don't need the
	 * lock. We just release it, call clock() and then reacquire it again.
	 */
	irq_spinlock_unlock(&irq->lock, false);
	clock();
	irq_spinlock_lock(&irq->lock, false);
}

void i8254_init(void)
{
	irq_initialize(&i8254_irq);
	i8254_irq.preack = true;
	i8254_irq.inr = IRQ_CLK;
	i8254_irq.claim = i8254_claim;
	i8254_irq.handler = i8254_irq_handler;
	irq_register(&i8254_irq);

	i8254_normal_operation();
}

void i8254_normal_operation(void)
{
	pio_write_8(CLK_PORT4, 0x36);
	i8259_disable_irqs(1 << IRQ_CLK);
	pio_write_8(CLK_PORT1, (CLK_CONST / HZ) & 0xf);
	pio_write_8(CLK_PORT1, (CLK_CONST / HZ) >> 8);
	i8259_enable_irqs(1 << IRQ_CLK);
}

void i8254_calibrate_delay_loop(void)
{
	/*
	 * One-shot timer. Count-down from 0xffff at 1193180Hz
	 * MAGIC_NUMBER is the magic value for 1ms.
	 */
	pio_write_8(CLK_PORT4, 0x30);
	pio_write_8(CLK_PORT1, 0xff);
	pio_write_8(CLK_PORT1, 0xff);

	uint8_t not_ok;
	uint32_t t1;
	uint32_t t2;

	do {
		/* will read both status and count */
		pio_write_8(CLK_PORT4, 0xc2);
		not_ok = (uint8_t) ((pio_read_8(CLK_PORT1) >> 6) & 1);
		t1 = pio_read_8(CLK_PORT1);
		t1 |= pio_read_8(CLK_PORT1) << 8;
	} while (not_ok);

	asm_delay_loop(LOOPS);

	pio_write_8(CLK_PORT4, 0xd2);
	t2 = pio_read_8(CLK_PORT1);
	t2 |= pio_read_8(CLK_PORT1) << 8;

	/*
	 * We want to determine the overhead of the calibrating mechanism.
	 */
	pio_write_8(CLK_PORT4, 0xd2);
	uint32_t o1 = pio_read_8(CLK_PORT1);
	o1 |= pio_read_8(CLK_PORT1) << 8;

	asm_fake_loop(LOOPS);

	pio_write_8(CLK_PORT4, 0xd2);
	uint32_t o2 = pio_read_8(CLK_PORT1);
	o2 |= pio_read_8(CLK_PORT1) << 8;

	uint32_t delta = (t1 - t2) - (o1 - o2);
	if (!delta)
		delta = 1;

	CPU->delay_loop_const =
	    ((MAGIC_NUMBER * LOOPS) / 1000) / delta +
	    (((MAGIC_NUMBER * LOOPS) / 1000) % delta ? 1 : 0);

	uint64_t clk1 = get_cycle();
	delay(1 << SHIFT);
	uint64_t clk2 = get_cycle();

	CPU->frequency_mhz = (clk2 - clk1) >> SHIFT;

	return;
}

/** @}
 */
