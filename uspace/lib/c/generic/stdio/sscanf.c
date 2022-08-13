/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/**
 * @file
 * @brief sscanf, vsscanf
 */

#include <stdarg.h>
#include <stdio.h>
#include "../private/stdio.h"
#include "../private/sstream.h"

int vsscanf(const char *s, const char *fmt, va_list ap)
{
	FILE f;

	__sstream_init(s, &f);
	return vfscanf(&f, fmt, ap);
}

int sscanf(const char *s, const char *fmt, ...)
{
	va_list args;
	int rc;

	va_start(args, fmt);
	rc = vsscanf(s, fmt, args);
	va_end(args);

	return rc;
}

/** @}
 */
