/*
 * Copyright (c) 2013 Martin Sucha
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

#ifndef LIBC_IO_KLOG_H_
#define LIBC_IO_KLOG_H_

#include <sys/types.h>
#include <stdarg.h>
#include <io/verify.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <abi/log.h>

extern size_t klog_write(log_level_t, const void *, size_t);
extern int klog_read(void *, size_t);

#define KLOG_PRINTF(lvl, fmt, ...) ({ \
	char *_fmt = str_dup(fmt); \
	size_t _fmtsize = str_size(_fmt); \
	if (_fmtsize > 0 && _fmt[_fmtsize - 1] == '\n') \
		_fmt[_fmtsize - 1] = 0; \
	char *_s; \
	int _c = asprintf(&_s, _fmt, ##__VA_ARGS__); \
	free(_fmt); \
	if (_c >= 0) { \
		_c = klog_write((lvl), _s, str_size(_s)); \
		free(_s); \
	}; \
	(_c >= 0); \
})

#endif

/** @}
 */
