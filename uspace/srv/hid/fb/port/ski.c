/*
 * Copyright (c) 2006 Ondrej Palkovsky
 * Copyright (c) 2008 Martin Decky
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

/** @file
 */

#include <sys/types.h>
#include <errno.h>
#include <sysinfo.h>
#include "../ctl/serial.h"
#include "ski.h"

#ifdef UARCH_ia64

#define SKI_PUTCHAR  31

/** Display character on ski debug console
 *
 * Use SSC (Simulator System Call) to
 * display character on debug console.
 *
 * @param c Character to be printed.
 *
 */
static void ski_putc(const char c)
{
	asm volatile (
		"mov r15 = %0\n"
		"mov r32 = %1\n"   /* r32 is in0 */
		"break 0x80000\n"  /* modifies r8 */
		:
		: "i" (SKI_PUTCHAR), "r" (c)
		: "r15", "in0", "r8"
	);
	
	if (c == '\n')
		ski_putc('\r');
}

static void ski_putchar(wchar_t ch)
{
	if ((ch >= 0) && (ch < 128))
		ski_putc(ch);
	else
		ski_putc('?');
}

static void ski_control_puts(const char *str)
{
	while (*str)
		ski_putc(*(str++));
}

int ski_init(void)
{
	sysarg_t present;
	int rc = sysinfo_get_value("fb", &present);
	if (rc != EOK)
		present = false;
	
	if (!present)
		return ENOENT;
	
	sysarg_t kind;
	rc = sysinfo_get_value("fb.kind", &kind);
	if (rc != EOK)
		kind = (sysarg_t) -1;
	
	if (kind != 6)
		return EINVAL;
	
	return serial_init(ski_putchar, ski_control_puts);
}

#else /* UARCH_ia64 */

int ski_init(void)
{
	return ENOENT;
}

#endif

/** @}
 */
