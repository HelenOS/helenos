/*
 * Copyright (c) 2008 Josef Cejka
 * Copyright (c) 2013 Vojtech Horky
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
/** @file Long jump implementation.
 *
 * Implementation inspired by Jiri Zarevucky's code from
 * http://bazaar.launchpad.net/~zarevucky-jiri/helenos/stdc/revision/1544/uspace/lib/posix/setjmp.h
 */

#ifndef LIBC_SETJMP_H_
#define LIBC_SETJMP_H_

#include <libarch/fibril.h>

struct jmp_buf_interal {
	context_t context;
	int return_value;
};
typedef struct jmp_buf_interal jmp_buf[1];

/*
 * Specified as extern to minimize number of included headers
 * because this file is used as is in libposix too.
 */
extern int context_save(context_t *ctx) __attribute__((returns_twice));

/**
 * Save current environment (registers).
 *
 * This function may return twice.
 *
 * @param env Variable where to save the environment (of type jmp_buf).
 * @return Whether the call returned after longjmp.
 * @retval 0 Environment was saved, normal execution.
 * @retval other longjmp was executed and returned here.
 */
#define setjmp(env) \
	((env)[0].return_value = 0, \
	context_save(&(env)[0].context), \
	(env)[0].return_value)

extern void longjmp(jmp_buf env, int val) __attribute__((noreturn));

#endif

/** @}
 */
