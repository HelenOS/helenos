/*
 * Copyright (c) 2006 Jakub Vana
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_IO_KIO_H_
#define _LIBC_IO_KIO_H_

#include <stddef.h>
#include <stdarg.h>
#include <io/verify.h>
#include <_bits/errno.h>
#include <_bits/size_t.h>

extern void __kio_init(void);
extern void __kio_fini(void);
extern errno_t kio_write(const void *, size_t, size_t *);
extern void kio_update(void);
extern void kio_command(const void *, size_t);
extern int kio_printf(const char *, ...)
    _HELENOS_PRINTF_ATTRIBUTE(1, 2);
extern int kio_vprintf(const char *, va_list);

/*
 * In some files, we have conditional DPRINTF(...) macro that is defined empty
 * in most cases. Provide a dummy printf so we get argument checking and
 * avoid unused variable errors.
 */

static inline int dummy_printf(const char *fmt, ...) _HELENOS_PRINTF_ATTRIBUTE(1, 2);
static inline int dummy_printf(const char *fmt, ...)
{
	/* Empty. */
	(void) fmt;
	return 0;
}

#endif

/** @}
 */
