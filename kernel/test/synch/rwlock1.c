/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

#include <test.h>
#include <arch.h>
#include <atomic.h>
#include <print.h>
#include <proc/thread.h>

#include <synch/waitq.h>
#include <synch/rwlock.h>

#define READERS		50
#define WRITERS		50

static rwlock_t rwlock;

char * test_rwlock1(bool quiet)
{
	rwlock_initialize(&rwlock);

	rwlock_write_lock(&rwlock);
	rwlock_write_unlock(&rwlock);	

	rwlock_read_lock(&rwlock);
	rwlock_read_lock(&rwlock);	
	rwlock_read_lock(&rwlock);
	rwlock_read_lock(&rwlock);
	rwlock_read_lock(&rwlock);

	rwlock_read_unlock(&rwlock);
	rwlock_read_unlock(&rwlock);	
	rwlock_read_unlock(&rwlock);
	rwlock_read_unlock(&rwlock);
	rwlock_read_unlock(&rwlock);
	
	rwlock_write_lock(&rwlock);
	rwlock_write_unlock(&rwlock);	

	rwlock_read_lock(&rwlock);
	rwlock_read_unlock(&rwlock);

	rwlock_write_lock(&rwlock);
	rwlock_write_unlock(&rwlock);	

	rwlock_read_lock(&rwlock);
	rwlock_read_unlock(&rwlock);
	
	return NULL;
}
