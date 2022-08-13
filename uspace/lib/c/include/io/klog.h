/*
 * SPDX-FileCopyrightText: 2013 Martin Sucha
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_IO_KLOG_H_
#define _LIBC_IO_KLOG_H_

#include <stddef.h>
#include <stdarg.h>
#include <io/verify.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <abi/log.h>

extern errno_t klog_write(log_level_t, const void *, size_t);
extern errno_t klog_read(void *, size_t, size_t *);

#define KLOG_PRINTF(lvl, fmt, ...) ({ \
	char *_s; \
	errno_t _rc = ENOMEM; \
	if (asprintf(&_s, fmt, ##__VA_ARGS__) >= 0) { \
		_rc = klog_write((lvl), _s, str_size(_s)); \
		free(_s); \
	}; \
	(_rc != EOK); \
})

#endif

/** @}
 */
