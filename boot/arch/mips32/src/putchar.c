/*
 * Copyright (c) 2006 Martin Decky
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

#include <stddef.h>
#include <stdint.h>
#include <arch/arch.h>
#include <putchar.h>
#include <str.h>

#ifdef PUTCHAR_ADDRESS
#undef PUTCHAR_ADDRESS
#endif

#if defined(MACHINE_msim)
#define _putchar(ch)	msim_putchar((ch))
static void msim_putchar(char ch)
{
	*((char *) MSIM_VIDEORAM_ADDRESS) = ch;
}
#endif

#if defined(MACHINE_lmalta) || defined(MACHINE_bmalta)
#define _putchar(ch)	yamon_putchar((ch))
typedef void (**yamon_print_count_ptr_t)(uint32_t, const char *, uint32_t);
yamon_print_count_ptr_t yamon_print_count =
    (yamon_print_count_ptr_t) YAMON_SUBR_PRINT_COUNT;

static void yamon_putchar(char ch)
{
	(*yamon_print_count)(0, &ch, 1);
}
#endif

void putuchar(const char32_t ch)
{
	if (ascii_check(ch))
		_putchar(ch);
	else
		_putchar('?');
}
