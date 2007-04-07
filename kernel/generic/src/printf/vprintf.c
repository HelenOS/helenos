/*
 * Copyright (c) 2006 Josef Cejka
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

/** @addtogroup generic	
 * @{
 */
/** @file
 */

#include <print.h>
#include <printf/printf_core.h>
#include <putchar.h>
#include <synch/spinlock.h>
#include <arch/asm.h>

SPINLOCK_INITIALIZE(printf_lock);			/**< vprintf spinlock */

static int vprintf_write(const char *str, size_t count, void *unused)
{
	size_t i;
	for (i = 0; i < count; i++)
		putchar(str[i]);
	return i;
}

int puts(const char *s)
{
	size_t i;
	for (i = 0; s[i] != 0; i++)
		putchar(s[i]);
	return i;
}

int vprintf(const char *fmt, va_list ap)
{
	struct printf_spec ps = {(int(*)(void *, size_t, void *)) vprintf_write, NULL};
	
	int irqpri = interrupts_disable();
	spinlock_lock(&printf_lock);
	
	int ret = printf_core(fmt, &ps, ap);
	
	spinlock_unlock(&printf_lock);
	interrupts_restore(irqpri);
	
	return ret;
}

/** @}
 */
