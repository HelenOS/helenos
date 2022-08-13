/*
 * SPDX-FileCopyrightText: 2013 Martin Sucha
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
