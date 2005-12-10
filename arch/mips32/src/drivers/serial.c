/*
 * Copyright (C) 2005 Ondrej Palkovsky
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

#include <interrupt.h>
#include <arch/cp0.h>
#include <arch/drivers/serial.h>
#include <console/chardev.h>
#include <console/console.h>

static chardev_t console;
static serial_t sconf[SERIAL_MAX];
static bool kb_enabled;

static void serial_write(chardev_t *d, const char ch)
{
	int i;
	serial_t *sd = (serial_t *)d->data;

	if (ch == '\n')
		serial_write(d, '\r');
	/* Wait until transmit buffer empty */
	while (! (SERIAL_READ_LSR(sd->port) & (1<<TRANSMIT_EMPTY_BIT)))
		;
	SERIAL_WRITE(sd->port, ch);
}

static void serial_enable(chardev_t *d)
{
	kb_enabled = true;
}

static void serial_disable(chardev_t *d)
{
	kb_enabled = false;
}

int serial_init(void)
{
	int i = 0;
	if (SERIAL_READ_LSR(SERIAL_COM1) == 0x60) {
		sconf[i].port = SERIAL_COM1;
		sconf[i].irq = SERIAL_COM1_IRQ;
		/* Enable interrupt on available data */
		i++;
	}
	return i;
}

static chardev_operations_t serial_ops = {
	.resume = serial_enable,
	.suspend = serial_disable,
	.write = serial_write
};

/** Process keyboard interrupt. Does not work in simics? */
static void serial_interrupt(int n, void *stack)
{
	serial_t *sd = (serial_t *)console.data;
	char ch;

	if (!(SERIAL_READ_LSR(sd->port) & 1))
		return;
	ch = SERIAL_READ(sd->port);

	if (ch =='\r')
		ch = '\n';
	chardev_push_character(&console, ch);
}


iroutine old_timer;
/** Do polling on timer interrupt */
static void timer_replace(int n, void *stack)
{
	old_timer(n, stack);
	serial_interrupt(n, stack);
}

#include <print.h>
chardev_t * serial_console(void)
{
	serial_t *sd = &sconf[0];


	chardev_initialize("serial_console", &console, &serial_ops);
	console.data = sd;
	kb_enabled = true;

//	exc_register(2, "serial_drvr", serial_interrupt);
	/* I don't know why, but the serial interrupts simply
	 * don't work on simics
	 */
	old_timer = exc_register(TIMER_IRQ, "serial_drvr_poll", timer_replace);
	

	return &console;
	
}
