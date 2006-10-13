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
 * @brief	NS 16550 serial port / keyboard driver.
 */

#include <genarch/kbd/ns16550.h>
#include <genarch/kbd/key.h>
#include <genarch/kbd/scanc.h>
#include <genarch/kbd/scanc_sun.h>
#include <arch/drivers/ns16550.h>
#include <irq.h>
#include <arch/interrupt.h>
#include <cpu.h>
#include <arch/asm.h>
#include <arch.h>
#include <typedefs.h>
#include <console/chardev.h>
#include <console/console.h>
#include <interrupt.h>
#include <sysinfo/sysinfo.h>

#define LSR_DATA_READY	0x01

/*
 * These codes read from ns16550 data register are silently ignored.
 */
#define IGNORE_CODE	0x7f		/* all keys up */

static void ns16550_suspend(chardev_t *);
static void ns16550_resume(chardev_t *);

static chardev_operations_t ops = {
	.suspend = ns16550_suspend,
	.resume = ns16550_resume,
	.read = ns16550_key_read
};

void ns16550_interrupt(int n, istate_t *istate);
void ns16550_wait(void);

/** Initialize keyboard and service interrupts using kernel routine */
void ns16550_grab(void)
{
	/* TODO */
}

/** Resume the former interrupt vector */
void ns16550_release(void)
{
	/* TODO */
}

/** Initialize ns16550. */
void ns16550_init(void)
{
	ns16550_grab();
	chardev_initialize("ns16550_kbd", &kbrd, &ops);
	stdin = &kbrd;
	
	sysinfo_set_item_val("kbd", NULL, true);
	sysinfo_set_item_val("kbd.irq", NULL, 0);
	sysinfo_set_item_val("kbd.address.virtual", NULL, (uintptr_t) kbd_virt_address);
	
	ns16550_ier_write(IER_ERBFI);				/* enable receiver interrupt */
	
	while (ns16550_lsr_read() & LSR_DATA_READY)
		(void) ns16550_rbr_read();
}

/** Process ns16550 interrupt.
 *
 * @param n Interrupt vector.
 * @param istate Interrupted state.
 */
void ns16550_interrupt(int n, istate_t *istate)
{
	/* TODO */
}

/** Wait until the controller reads its data. */
void ns16550_wait(void)
{
}

/* Called from getc(). */
void ns16550_resume(chardev_t *d)
{
}

/* Called from getc(). */
void ns16550_suspend(chardev_t *d)
{
}

char ns16550_key_read(chardev_t *d)
{
	char ch;	

	while(!(ch = active_read_buff_read())) {
		uint8_t x;
		while (!(ns16550_lsr_read() & LSR_DATA_READY))
			;
		x = ns16550_rbr_read();
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
void ns16550_poll(void)
{
	uint8_t x;

	while (ns16550_lsr_read() & LSR_DATA_READY) {
		x = ns16550_rbr_read();
		if (x != IGNORE_CODE) {
			if (x & KEY_RELEASE)
				key_released(x ^ KEY_RELEASE);
			else
				key_pressed(x);
		}
	}
}

irq_ownership_t ns16550_claim(void)
{
	/* TODO */
	return IRQ_ACCEPT;
}

void ns16550_irq_handler(irq_t *irq, void *arg, ...)
{
	panic("Not yet implemented.\n");
}

/** @}
 */
