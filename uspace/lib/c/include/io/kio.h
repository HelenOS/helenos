/*
 * SPDX-FileCopyrightText: 2006 Jakub Vana
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
