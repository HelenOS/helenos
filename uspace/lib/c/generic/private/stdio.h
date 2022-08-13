/*
 * SPDX-FileCopyrightText: 2011 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_PRIVATE_STDIO_H_
#define _LIBC_PRIVATE_STDIO_H_

#include <adt/list.h>
#include <stdio.h>
#include <async.h>
#include <stddef.h>
#include <offset.h>

/** Maximum characters that can be pushed back by ungetc() */
#define UNGETC_MAX 1

/** Stream operations */
typedef struct {
	/** Read from stream */
	size_t (*read)(void *buf, size_t size, size_t nmemb, FILE *stream);
	/** Write to stream */
	size_t (*write)(const void *buf, size_t size, size_t nmemb,
	    FILE *stream);
	/** Flush stream */
	int (*flush)(FILE *stream);
} __stream_ops_t;

enum __buffer_state {
	/** Buffer is empty */
	_bs_empty,

	/** Buffer contains data to be written */
	_bs_write,

	/** Buffer contains prefetched data for reading */
	_bs_read
};

struct _IO_FILE {
	/** Linked list pointer. */
	link_t link;

	/** Stream operations */
	__stream_ops_t *ops;

	/** Underlying file descriptor. */
	int fd;

	/** Instance argument */
	void *arg;

	/** File position. */
	aoff64_t pos;

	/** Error indicator. */
	int error;

	/** End-of-file indicator. */
	int eof;

	/** Session to the file provider */
	async_sess_t *sess;

	/**
	 * Non-zero if the stream needs sync on fflush(). XXX change
	 * console semantics so that sync is not needed.
	 */
	int need_sync;

	/** Buffering type */
	enum __buffer_type btype;

	/** Buffer */
	uint8_t *buf;

	/** Buffer size */
	size_t buf_size;

	/** Buffer state */
	enum __buffer_state buf_state;

	/** Buffer I/O pointer */
	uint8_t *buf_head;

	/** Points to end of occupied space when in read mode. */
	uint8_t *buf_tail;

	/** Pushed back characters */
	uint8_t ungetc_buf[UNGETC_MAX];

	/** Number of pushed back characters */
	int ungetc_chars;
};

#endif

/** @}
 */
