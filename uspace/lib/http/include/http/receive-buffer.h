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

/** @addtogroup http
 * @{
 */
/**
 * @file
 */

#ifndef HTTP_RECEIVE_BUFFER_H_
#define HTTP_RECEIVE_BUFFER_H_

#include <adt/list.h>
#include <stddef.h>
#include <types/common.h>

/** Receive data.
 *
 * @param client_data client data
 * @param buf buffer to store the data
 * @param buf_size buffer size
 * @param nrecv number of bytes actually received
 * @return EOK on success or an error code
 */
typedef errno_t (*receive_func_t)(void *, void *, size_t, size_t *);

typedef struct {
	size_t size;
	char *buffer;
	size_t in;
	size_t out;
	
	void *client_data;
	receive_func_t receive;
	
	list_t marks;
} receive_buffer_t;

typedef struct {
	link_t link;
	size_t offset;
} receive_buffer_mark_t;

typedef bool (*char_class_func_t)(char);

extern errno_t recv_buffer_init(receive_buffer_t *, size_t, receive_func_t, void *);
extern errno_t recv_buffer_init_const(receive_buffer_t *, void *, size_t);
extern void recv_buffer_fini(receive_buffer_t *);
extern void recv_reset(receive_buffer_t *);
extern void recv_mark(receive_buffer_t *, receive_buffer_mark_t *);
extern void recv_unmark(receive_buffer_t *, receive_buffer_mark_t *);
extern void recv_mark_update(receive_buffer_t *, receive_buffer_mark_t *);
extern errno_t recv_cut(receive_buffer_t *, receive_buffer_mark_t *,
    receive_buffer_mark_t *, void **, size_t *);
extern errno_t recv_cut_str(receive_buffer_t *, receive_buffer_mark_t *,
    receive_buffer_mark_t *, char **);
extern errno_t recv_char(receive_buffer_t *, char *, bool);
extern errno_t recv_buffer(receive_buffer_t *, char *, size_t, size_t *);
extern errno_t recv_discard(receive_buffer_t *, char, size_t *);
extern errno_t recv_discard_str(receive_buffer_t *, const char *, size_t *);
extern errno_t recv_while(receive_buffer_t *, char_class_func_t);
extern errno_t recv_eol(receive_buffer_t *, size_t *);
extern errno_t recv_line(receive_buffer_t *, char *, size_t, size_t *);

#endif

/** @}
 */
