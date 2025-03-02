/*
 * Copyright (c) 2025 Jiri Svoboda
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

#include <errno.h>
#include <fibril.h>
#include <fibril_synch.h>
//#include <stdio.h>
//#include <stddef.h>
#include "../tester.h"

static fibril_mutex_t fm1, fm2;

static errno_t fibril_fn(void *data)
{
	TPRINTF("F2: Lock M2\n");
	fibril_mutex_lock(&fm2);
	fibril_sleep(1);
	TPRINTF("F2: Lock M1\n");
	fibril_mutex_lock(&fm1);
	TPRINTF("F2: Unlock M1, M2\n");
	fibril_mutex_unlock(&fm1);
	fibril_mutex_unlock(&fm2);
	return EOK;
}

const char *test_deadlock(void)
{
	fid_t fid;

	fibril_mutex_initialize(&fm1);
	fibril_mutex_initialize(&fm2);

	TPRINTF("Creating fibril\n");
	fid = fibril_create(fibril_fn, NULL);
	if (fid == 0) {
		TPRINTF("\nCould not create fibril.\n");
		goto error;
	}

	fibril_add_ready(fid);

	TPRINTF("F1: Lock M1\n");
	fibril_mutex_lock(&fm1);
	fibril_sleep(1);
	TPRINTF("F1: Lock M2\n");
	fibril_mutex_lock(&fm2);

	TPRINTF("F1: Unlock M2, M1\n");
	fibril_mutex_unlock(&fm2);
	fibril_mutex_unlock(&fm1);

	return NULL;
error:
	return "Test failed";
}
