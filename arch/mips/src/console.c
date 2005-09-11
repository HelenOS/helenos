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

#include <putchar.h>
#include <arch/types.h>
#include <arch/cp0.h>
#include <arch/console.h>
#include <arch.h>
#include <arch/drivers/arc.h>
#include <arch/arch.h>

/** Putchar that works with MSIM & gxemul */
static void cons_putchar(const char ch)
{
	*((char *) VIDEORAM) = ch;
}

/** Putchar that works with simics */
static void serial_putchar(const char ch)
{
	int i;

	if (ch=='\n')
		putchar('\r');

	/* Wait until transmit buffer empty */
	while (! ((*SERIAL_LSR) & (1<<TRANSMIT_EMPTY_BIT)))
		;
	*(SERIAL_PORT_BASE) = ch;
}

static void (*putchar_func)(const char ch) = cons_putchar;

void console_init(void)
{
	if (arc_enabled()) 
		putchar_func = arc_putchar;
	/* The LSR on the start usually contains this value */
	else if (*SERIAL_LSR == 0x60)
		putchar_func = serial_putchar;
	else
		putchar_func = cons_putchar;
}

void putchar(const char ch)
{
	putchar_func(ch);
}
