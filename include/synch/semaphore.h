/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__

#include <arch/types.h>
#include <typedefs.h>
#include <synch/waitq.h>
#include <synch/synch.h>

struct semaphore
{
    waitq_t wq;
};

#define semaphore_down(s) \
	_semaphore_down_timeout((s),SYNCH_NO_TIMEOUT,SYNCH_BLOCKING)
#define semaphore_trydown(s) \
	_semaphore_down_timeout((s),SYNCH_NO_TIMEOUT,SYNCH_NON_BLOCKING)	
#define semaphore_down_timeout(s,usec) \
	_semaphore_down_timeout((s),(usec),SYNCH_NON_BLOCKING)

extern void semaphore_initialize(semaphore_t *s, int val);
extern int _semaphore_down_timeout(semaphore_t *s, __u32 usec, int trydown);
extern void semaphore_up(semaphore_t *s);

#endif
