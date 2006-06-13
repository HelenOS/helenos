/*
 * Copyright (C) 2001-2004 Jakub Jermar
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
/** @file
 */

#include <arch/types.h>
#include <time/clock.h>
#include <time/delay.h>
#include <arch/interrupt.h>
#include <arch/drivers/i8259.h>
#include <arch/drivers/i8254.h>
#include <cpu.h>
#include <config.h>
#include <arch/pm.h>
#include <arch/asm.h>
#include <arch/cpuid.h>
#include <arch.h>
#include <time/delay.h>
#include <interrupt.h>

/*
 * i8254 chip driver.
 * Low level time functions.
 */

#define CLK_PORT1	0x40
#define CLK_PORT4	0x43


#define CLK_CONST	1193180
#define MAGIC_NUMBER	1194

static void i8254_interrupt(int n, istate_t *istate);

void i8254_init(void)
{
	i8254_normal_operation();
}

void i8254_normal_operation(void)
{
	outb(CLK_PORT4, 0x36);
	pic_disable_irqs(1<<IRQ_CLK);
	outb(CLK_PORT1, (CLK_CONST/HZ) & 0xf);
	outb(CLK_PORT1, (CLK_CONST/HZ) >> 8);
	pic_enable_irqs(1<<IRQ_CLK);
	exc_register(VECTOR_CLK, "i8254_clock", (iroutine) i8254_interrupt);
}

#define LOOPS 150000
#define SHIFT 11
void i8254_calibrate_delay_loop(void)
{
	__u64 clk1, clk2;
	__u32 t1, t2, o1, o2;
	__u8 not_ok;


	/*
	 * One-shot timer. Count-down from 0xffff at 1193180Hz
	 * MAGIC_NUMBER is the magic value for 1ms.
	 */
	outb(CLK_PORT4, 0x30);
	outb(CLK_PORT1, 0xff);
	outb(CLK_PORT1, 0xff);

	do {
		/* will read both status and count */
		outb(CLK_PORT4, 0xc2);
		not_ok = (inb(CLK_PORT1)>>6)&1;
		t1 = inb(CLK_PORT1);
		t1 |= inb(CLK_PORT1) << 8;
	} while (not_ok);

	asm_delay_loop(LOOPS);

	outb(CLK_PORT4, 0xd2);
	t2 = inb(CLK_PORT1);
	t2 |= inb(CLK_PORT1) << 8;

	/*
	 * We want to determine the overhead of the calibrating mechanism.
	 */
	outb(CLK_PORT4, 0xd2);
	o1 = inb(CLK_PORT1);
	o1 |= inb(CLK_PORT1) << 8;

	asm_fake_loop(LOOPS);

	outb(CLK_PORT4, 0xd2);
	o2 = inb(CLK_PORT1);
	o2 |= inb(CLK_PORT1) << 8;

	CPU->delay_loop_const = ((MAGIC_NUMBER*LOOPS)/1000) / ((t1-t2)-(o1-o2)) + (((MAGIC_NUMBER*LOOPS)/1000) % ((t1-t2)-(o1-o2)) ? 1 : 0);

	clk1 = rdtsc();
	delay(1<<SHIFT);
	clk2 = rdtsc();
	
	CPU->frequency_mhz = (clk2-clk1)>>SHIFT;

	return;
}

void i8254_interrupt(int n, istate_t *istate)
{
	trap_virtual_eoi();
	clock();
}

 /** @}
 */

