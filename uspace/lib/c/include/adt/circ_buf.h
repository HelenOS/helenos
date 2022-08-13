/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file Circular buffer
 */

#ifndef _LIBC_CIRC_BUF_H_
#define _LIBC_CIRC_BUF_H_

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
