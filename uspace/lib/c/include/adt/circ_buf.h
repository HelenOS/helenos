/*
 * Copyright (c) 2017 Jiri Svoboda
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
/** @file Circular buffer
 */

#ifndef LIBC_CIRC_BUF_H_
#define LIBC_CIRC_BUF_H_

#include <errno.h>
#include <stddef.h>

/** Circular buffer */
typedef struct {
	/** Buffer */
	void *buf;
	/** Number of buffer members */
	size_t nmemb;
	/** Member size */
	size_t size;
	/** Read position */
	size_t rp;
	/** Write position */
	size_t wp;
	/** Number of used entries */
	size_t nused;
} circ_buf_t;

extern void circ_buf_init(circ_buf_t *, void *, size_t, size_t);
extern size_t circ_buf_nfree(circ_buf_t *);
extern size_t circ_buf_nused(circ_buf_t *);
extern errno_t circ_buf_push(circ_buf_t *, const void *);
extern errno_t circ_buf_pop(circ_buf_t *, void *);

#endif

/** @}
 */
