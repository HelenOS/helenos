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
 *
 * Note that this file is derived from the i8042.c.
 * The i8042 driver could be persuaded to control
 * the z8530 at least in the polling mode.
 * As a result, this file may contain inaccurate
 * and z8530-irrelevant constants, code and comments.
 * Still it miraculously works.
 */

#include <genarch/kbd/z8530.h>
#include <genarch/kbd/key.h>
#include <genarch/kbd/scanc.h>
#include <genarch/kbd/scanc_sun.h>
#include <arch/drivers/z8530.h>
#include <arch/interrupt.h>
#include <cpu.h>
#include <arch/asm.h>
#include <arch.h>
#include <typedefs.h>
#include <console/chardev.h>
#include <console/console.h>
#include <interrupt.h>

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

#define z8530_SET_COMMAND 	0x60
#define z8530_COMMAND 		0x69

#define z8530_BUFFER_FULL_MASK	0x01
#define z8530_WAIT_MASK 	0x02
#define z8530_MOUSE_DATA        0x20

/*
 * These codes read from z8530 data register are silently ignored.
 */
#define IGNORE_CODE	0x7f		/* all keys up */

static void z8530_suspend(chardev_t *);
static void z8530_resume(chardev_t *);

chardev_t kbrd;
static chardev_operations_t ops = {
	.suspend = z8530_suspend,
	.resume = z8530_resume,
	.read = key_read
};

static void z8530_interrupt(int n, istate_t *istate);
static void z8530_wait(void);

static iroutine oldvector;
/** Initialize keyboard and service interrupts using kernel routine */
void z8530_grab(void)
{
	oldvector = exc_register(VECTOR_KBD, "z8530_interrupt", (iroutine) z8530_interrupt);
	z8530_wait();
	z8530_command_write(z8530_SET_COMMAND);
	z8530_wait();
	z8530_data_write(z8530_COMMAND);
	z8530_wait();
}
/** Resume the former interrupt vector */
void z8530_release(void)
{
	if (oldvector)
		exc_register(VECTOR_KBD, "user_interrupt", oldvector);
}

/** Initialize z8530. */
void z8530_init(void)
{
	int i;

	z8530_grab();
        /* Prevent user from accidentaly releasing calling z8530_resume
	 * and disabling keyboard 
	 */
	oldvector = NULL; 

	trap_virtual_enable_irqs(1<<IRQ_KBD);
	chardev_initialize("z8530_kbd", &kbrd, &ops);
	stdin = &kbrd;

	/*
	 * Clear input buffer.
	 * Number of iterations is limited to prevent infinite looping.
	 */
	for (i = 0; (z8530_status_read() & z8530_BUFFER_FULL_MASK) && i < 100; i++) {
		z8530_data_read();
	}  
}

/** Process z8530 interrupt.
 *
 * @param n Interrupt vector.
 * @param istate Interrupted state.
 */
void z8530_interrupt(int n, istate_t *istate)
{
	uint8_t x;
	uint8_t status;

	while (((status=z8530_status_read()) & z8530_BUFFER_FULL_MASK)) {
		x = z8530_data_read();

		if ((status & z8530_MOUSE_DATA))
			continue;

		if (x & KEY_RELEASE)
			key_released(x ^ KEY_RELEASE);
		else
			key_pressed(x);
	}
	trap_virtual_eoi();
}

/** Wait until the controller reads its data. */
void z8530_wait(void) {
	while (z8530_status_read() & z8530_WAIT_MASK) {
		/* wait */
	}
}

/* Called from getc(). */
void z8530_resume(chardev_t *d)
{
}

/* Called from getc(). */
void z8530_suspend(chardev_t *d)
{
}

char key_read(chardev_t *d)
{
	char ch;	

	while(!(ch = active_read_buff_read())) {
		uint8_t x;
		while (!(z8530_status_read() & z8530_BUFFER_FULL_MASK))
			;
		x = z8530_data_read();
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

	while (((x = z8530_status_read() & z8530_BUFFER_FULL_MASK))) {
		x = z8530_data_read();
		if (x != IGNORE_CODE) {
			if (x & KEY_RELEASE)
				key_released(x ^ KEY_RELEASE);
			else
				key_pressed(x);
		}
	}
}

/** @}
 */
