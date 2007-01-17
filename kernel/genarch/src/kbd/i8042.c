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

/** @addtogroup genarch	
 * @{
 */
/**
 * @file
 * @brief	i8042 processor driver.
 *
 * It takes care of low-level keyboard functions.
 */

#include <genarch/kbd/i8042.h>
#include <genarch/kbd/key.h>
#include <genarch/kbd/scanc.h>
#include <genarch/kbd/scanc_pc.h>
#include <arch/drivers/i8042.h>
#include <cpu.h>
#include <arch/asm.h>
#include <arch.h>
#include <typedefs.h>
#include <console/chardev.h>
#include <console/console.h>
#include <interrupt.h>
#include <sysinfo/sysinfo.h>
#include <ddi/irq.h>

/* Keyboard commands. */
#define KBD_ENABLE	0xf4
#define KBD_DISABLE	0xf5
#define KBD_ACK		0xfa

/*
 * 60  Write 8042 Command Byte: next data byte written to port 60h is
 *     placed in 8042 command register. Format:
 *
 *    |7|6|5|4|3|2|1|0|8042 Command Byte
 *     | | | | | | | `---- 1=enable output register full interrupt
 *     | | | | | | `----- should be 0
 *     | | | | | `------ 1=set status register system, 0=clear
 *     | | | | `------- 1=override keyboard inhibit, 0=allow inhibit
 *     | | | `-------- disable keyboard I/O by driving clock line low
 *     | | `--------- disable auxiliary device, drives clock line low
 *     | `---------- IBM scancode translation 0=AT, 1=PC/XT
 *     `----------- reserved, should be 0
 */

#define i8042_SET_COMMAND 	0x60
#define i8042_COMMAND 		0x69

#define i8042_BUFFER_FULL_MASK	0x01
#define i8042_WAIT_MASK 	0x02
#define i8042_MOUSE_DATA        0x20

static void i8042_suspend(chardev_t *);
static void i8042_resume(chardev_t *);

static chardev_operations_t ops = {
	.suspend = i8042_suspend,
	.resume = i8042_resume,
	.read = i8042_key_read
};

/** Structure for i8042's IRQ. */
static irq_t i8042_kbd_irq;
static irq_t i8042_mouse_irq;

/** Wait until the controller reads its data. */
static void i8042_wait(void) {
	while (i8042_status_read() & i8042_WAIT_MASK) {
		/* wait */
	}
}

void i8042_grab(void)
{
	ipl_t ipl = interrupts_disable();
	
	i8042_wait();
	i8042_command_write(i8042_SET_COMMAND);
	i8042_wait();
	i8042_data_write(i8042_COMMAND);
	i8042_wait();

	spinlock_lock(&i8042_kbd_irq.lock);
	i8042_kbd_irq.notif_cfg.notify = false;
	spinlock_unlock(&i8042_kbd_irq.lock);
	
	spinlock_lock(&i8042_mouse_irq.lock);
	i8042_mouse_irq.notif_cfg.notify = false;
	spinlock_unlock(&i8042_mouse_irq.lock);
	
	interrupts_restore(ipl);
}

void i8042_release(void)
{
	ipl_t ipl = interrupts_disable();
	
	spinlock_lock(&i8042_kbd_irq.lock);
	if (i8042_kbd_irq.notif_cfg.answerbox)
		i8042_kbd_irq.notif_cfg.notify = true;
	spinlock_unlock(&i8042_kbd_irq.lock);
	
	spinlock_lock(&i8042_mouse_irq.lock);
	if (i8042_mouse_irq.notif_cfg.answerbox)
		i8042_mouse_irq.notif_cfg.notify = true;
	spinlock_unlock(&i8042_mouse_irq.lock);
	
	interrupts_restore(ipl);
}

static irq_ownership_t i8042_claim(void)
{
	return IRQ_ACCEPT;
}

static void i8042_kbd_irq_handler(irq_t *irq, void *arg, ...)
{
	if (irq->notif_cfg.notify && irq->notif_cfg.answerbox)
		ipc_irq_send_notif(irq);
	else {
		uint8_t x;
		uint8_t status;
		
		while (((status = i8042_status_read()) & i8042_BUFFER_FULL_MASK)) {
			x = i8042_data_read();
			
			if ((status & i8042_MOUSE_DATA))
				continue;
			
			if (x & KEY_RELEASE)
				key_released(x ^ KEY_RELEASE);
			else
				key_pressed(x);
		}
	}
}

static void i8042_mouse_irq_handler(irq_t *irq, void *arg, ...)
{
	if (irq->notif_cfg.notify && irq->notif_cfg.answerbox)
		ipc_irq_send_notif(irq);
}

/** Initialize i8042. */
void i8042_init(devno_t kbd_devno, inr_t kbd_inr, devno_t mouse_devno, inr_t mouse_inr)
{
	chardev_initialize("i8042_kbd", &kbrd, &ops);
	stdin = &kbrd;
	
	irq_initialize(&i8042_kbd_irq);
	i8042_kbd_irq.devno = kbd_devno;
	i8042_kbd_irq.inr = kbd_inr;
	i8042_kbd_irq.claim = i8042_claim;
	i8042_kbd_irq.handler = i8042_kbd_irq_handler;
	irq_register(&i8042_kbd_irq);
	
	irq_initialize(&i8042_mouse_irq);
	i8042_mouse_irq.devno = mouse_devno;
	i8042_mouse_irq.inr = mouse_inr;
	i8042_mouse_irq.claim = i8042_claim;
	i8042_mouse_irq.handler = i8042_mouse_irq_handler;
	irq_register(&i8042_mouse_irq);
	
	trap_virtual_enable_irqs(1 << kbd_inr);
	trap_virtual_enable_irqs(1 << mouse_inr);
	
	/*
	 * Clear input buffer.
	 * Number of iterations is limited to prevent infinite looping.
	 */
	int i;
	for (i = 0; (i8042_status_read() & i8042_BUFFER_FULL_MASK) && i < 100; i++) {
		i8042_data_read();
	}
	
	sysinfo_set_item_val("kbd", NULL, true);
	sysinfo_set_item_val("kbd.devno", NULL, kbd_devno);
	sysinfo_set_item_val("kbd.inr", NULL, kbd_inr);
	
	sysinfo_set_item_val("mouse", NULL, true);
	sysinfo_set_item_val("mouse.devno", NULL, mouse_devno);
	sysinfo_set_item_val("mouse.inr", NULL, mouse_inr);
	
	i8042_grab();
}

/* Called from getc(). */
void i8042_resume(chardev_t *d)
{
}

/* Called from getc(). */
void i8042_suspend(chardev_t *d)
{
}

char i8042_key_read(chardev_t *d)
{
	char ch;	

	while(!(ch = active_read_buff_read())) {
		uint8_t x;
		while (!(i8042_status_read() & i8042_BUFFER_FULL_MASK))
			;
		x = i8042_data_read();
		if (x & KEY_RELEASE)
			key_released(x ^ KEY_RELEASE);
		else
			active_read_key_pressed(x);
	}
	return ch;
}

/** Poll for key press and release events.
 *
 * This function can be used to implement keyboard polling.
 */
void i8042_poll(void)
{
	uint8_t x;

	while (((x = i8042_status_read() & i8042_BUFFER_FULL_MASK))) {
		x = i8042_data_read();
		if (x & KEY_RELEASE)
			key_released(x ^ KEY_RELEASE);
		else
			key_pressed(x);
	}
}

/** @}
 */
