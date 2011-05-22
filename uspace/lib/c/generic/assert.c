/*
 * Copyright (c) 2011 Martin Decky
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

#include <assert.h>
#include <stdio.h>
#include <io/klog.h>
#include <stdlib.h>
#include <atomic.h>
#include <stacktrace.h>
#include <stdint.h>

#define MSG_START	"Assertion failed ("
#define MSG_FILE	") in file \""
#define MSG_LINE	"\", line "
#define MSG_END		".\n"

static atomic_t failed_asserts;

void assert_abort(const char *cond, const char *file, unsigned int line)
{
	char lstr[11];
	char *pd = &lstr[10];
	uint32_t ln = (uint32_t) line;

	/*
	 * Convert ln to a zero-terminated string.
	 */
	*pd-- = 0;
	for (; ln; ln /= 10)
		*pd-- = '0' + ln % 10;
	pd++;

	/*
	 * Send the message safely to klog. Nested asserts should not occur.
	 */
	klog_write(MSG_START, str_size(MSG_START));
	klog_write(cond, str_size(cond));
	klog_write(MSG_FILE, str_size(MSG_FILE));
	klog_write(file, str_size(file));
	klog_write(MSG_LINE, str_size(MSG_LINE));
	klog_write(pd, str_size(pd));
	klog_write(MSG_END, str_size(MSG_END));

	/*
	 * Check if this is a nested or parallel assert.
	 */
	if (atomic_postinc(&failed_asserts))
		abort();
	
	/*
	 * Attempt to print the message to standard output and display
	 * the stack trace. These operations can theoretically trigger nested
	 * assertions.
	 */
	printf(MSG_START "%s" MSG_FILE "%s" MSG_LINE "%u" MSG_END,
	    cond, file, line);
	stacktrace_print();

	abort();
}

/** @}
 */
