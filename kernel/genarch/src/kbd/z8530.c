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

/** @addtogroup genarch	
 * @{
 */
/**
 * @file
 * @brief	Zilog 8530 serial port / keyboard driver.
 */

#include <genarch/kbd/z8530.h>
#include <genarch/kbd/key.h>
#include <genarch/kbd/scanc.h>
#include <genarch/kbd/scanc_sun.h>
#include <arch/drivers/z8530.h>
#include <irq.h>
#include <arch/interrupt.h>
#include <arch/drivers/kbd.h>
#include <arch/drivers/fhc.h>
#include <cpu.h>
#include <arch/asm.h>
#include <arch.h>
#include <typedefs.h>
#include <console/chardev.h>
#include <console/console.h>
#include <interrupt.h>
#include <sysinfo/sysinfo.h>
#include <print.h>

/*
 * These codes read from z8530 data register are silently ignored.
 */
#define IGNORE_CODE	0x7f		/* all keys up */

bool z8530_belongs_to_kernel = true;

static void z8530_suspend(chardev_t *);
static void z8530_resume(chardev_t *);

static chardev_operations_t ops = {
	.suspend = z8530_suspend,
	.resume = z8530_resume,
	.read = z8530_key_read
};

void z8530_wait(void);

/** Initialize keyboard and service interrupts using kernel routine */
void z8530_grab(void)
{
	z8530_belongs_to_kernel = true;
}

/** Resume the former interrupt vector */
void z8530_release(void)
{
	z8530_belongs_to_kernel = false;
}

/** Initialize z8530. */
void z8530_init(void)
{
	chardev_initialize("z8530_kbd", &kbrd, &ops);
	stdin = &kbrd;

	sysinfo_set_item_val("kbd", NULL, true);
	sysinfo_set_item_val("kbd.irq", NULL, 0);
	sysinfo_set_item_val("kbd.address.virtual", NULL, (uintptr_t) kbd_virt_address);

	(void) z8530_read_a(RR8);

	/*
	 * Clear any pending TX interrupts or we never manage
	 * to set FHC UART interrupt state to idle.
	 */
	z8530_write_a(WR0, WR0_TX_IP_RST);

	z8530_write_a(WR1, WR1_IARCSC);		/* interrupt on all characters */

	/* 8 bits per character and enable receiver */
	z8530_write_a(WR3, WR3_RX8BITSCH | WR3_RX_ENABLE);
	
	z8530_write_a(WR9, WR9_MIE);		/* Master Interrupt Enable. */
}

/** Process z8530 interrupt.
 *
 * @param n Interrupt vector.
 * @param istate Interrupted state.
 */
void z8530_interrupt(void)
{
	z8530_poll();
}

/** Wait until the controller reads its data. */
void z8530_wait(void) {
}

/* Called from getc(). */
void z8530_resume(chardev_t *d)
{
}

/* Called from getc(). */
void z8530_suspend(chardev_t *d)
{
}

char z8530_key_read(chardev_t *d)
{
	char ch;	

	while(!(ch = active_read_buff_read())) {
		uint8_t x;
		while (!(z8530_read_a(RR0) & RR0_RCA))
			;
		x = z8530_read_a(RR8);
		if (x != IGNORE_CODE) {
			if (x & KEY_RELEASE)
				key_released(x ^ KEY_RELEASE);
			else
				active_read_key_pressed(x);
		}
	}
	return ch;
}

/** Poll for key press and release events.
 *
 * This function can be used to implement keyboard polling.
 */
void z8530_poll(void)
{
	uint8_t x;

	while (z8530_read_a(RR0) & RR0_RCA) {
		x = z8530_read_a(RR8);
		if (x != IGNORE_CODE) {
			if (x & KEY_RELEASE)
				key_released(x ^ KEY_RELEASE);
			else
				key_pressed(x);
		}
	}
}

irq_ownership_t z8530_claim(void)
{
	return (z8530_read_a(RR0) & RR0_RCA);
}

void z8530_irq_handler(irq_t *irq, void *arg, ...)
{
	/*
	 * So far, we know we got this interrupt through the FHC.
	 * Since we don't have enough information about the FHC and
	 * because the interrupt looks like level sensitive,
	 * we cannot handle it by scheduling one of the level
	 * interrupt traps. Process the interrupt directly.
	 */
	if (z8530_belongs_to_kernel)
		z8530_interrupt();
	else
		ipc_irq_send_notif(0);
	fhc_clear_interrupt(central_fhc, irq->inr);
}

/** @}
 */
