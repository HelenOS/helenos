/*
 * Copyright (c) 2005 Jakub Jermar
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

/** @file Ski console driver.
 */

#include <async.h>
#include <ddf/driver.h>
#include <ddf/log.h>
#include <errno.h>
#include <fibril.h>
#include <io/chardev.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "ski-con.h"

#define SKI_GETCHAR		21
#define SKI_PUTCHAR		31

#define POLL_INTERVAL		10000

static errno_t ski_con_fibril(void *arg);
static int32_t ski_con_getchar(void);
static void ski_con_connection(cap_call_handle_t, ipc_call_t *, void *);

static errno_t ski_con_read(chardev_srv_t *, void *, size_t, size_t *);
static errno_t ski_con_write(chardev_srv_t *, const void *, size_t, size_t *);

static chardev_ops_t ski_con_chardev_ops = {
	.read = ski_con_read,
	.write = ski_con_write
};

static void ski_con_putchar(ski_con_t *con, char ch); /* XXX */

/** Add ski console device. */
errno_t ski_con_add(ski_con_t *con)
{
	fid_t fid;
	ddf_fun_t *fun = NULL;
	bool bound = false;
	errno_t rc;

	circ_buf_init(&con->cbuf, con->buf, ski_con_buf_size, 1);
	fibril_mutex_initialize(&con->buf_lock);
	fibril_condvar_initialize(&con->buf_cv);

	fun = ddf_fun_create(con->dev, fun_exposed, "a");
	if (fun == NULL) {
		ddf_msg(LVL_ERROR, "Error creating function 'a'.");
		rc = ENOMEM;
		goto error;
	}

	ddf_fun_set_conn_handler(fun, ski_con_connection);

	chardev_srvs_init(&con->cds);
	con->cds.ops = &ski_con_chardev_ops;
	con->cds.sarg = con;

	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error binding function 'a'.");
		goto error;
	}

	ddf_fun_add_to_category(fun, "console");

	bound = true;

	fid = fibril_create(ski_con_fibril, con);
	if (fid == 0) {
		ddf_msg(LVL_ERROR, "Error creating fibril.");
		rc = ENOMEM;
		goto error;
	}

	fibril_add_ready(fid);
	return EOK;
error:
	if (bound)
		ddf_fun_unbind(fun);
	if (fun != NULL)
		ddf_fun_destroy(fun);

	return rc;
}

/** Remove ski console device */
errno_t ski_con_remove(ski_con_t *con)
{
	return ENOTSUP;
}

/** Ski console device gone */
errno_t ski_con_gone(ski_con_t *con)
{
	return ENOTSUP;
}

/** Poll Ski for keypresses. */
static errno_t ski_con_fibril(void *arg)
{
	int32_t c;
	ski_con_t *con = (ski_con_t *) arg;
	errno_t rc;

	while (true) {
		while (true) {
			c = ski_con_getchar();
			if (c == 0)
				break;

			fibril_mutex_lock(&con->buf_lock);

			rc = circ_buf_push(&con->cbuf, &c);
			if (rc != EOK)
				ddf_msg(LVL_ERROR, "Buffer overrun");

			fibril_mutex_unlock(&con->buf_lock);
			fibril_condvar_broadcast(&con->buf_cv);
		}

		async_usleep(POLL_INTERVAL);
	}

	return 0;
}

/** Ask Ski if a key was pressed.
 *
 * Use SSC (Simulator System Call) to get character from the debug console.
 * This call is non-blocking.
 *
 * @return ASCII code of pressed key or 0 if no key pressed.
 */
static int32_t ski_con_getchar(void)
{
	uint64_t ch;

#ifdef UARCH_ia64
	asm volatile (
	    "mov r15 = %1\n"
	    "break 0x80000;;\n"	/* modifies r8 */
	    "mov %0 = r8;;\n"

	    : "=r" (ch)
	    : "i" (SKI_GETCHAR)
	    : "r15", "r8"
	);
#else
	ch = 0;
#endif
	return (int32_t) ch;
}


/** Display character on ski debug console
 *
 * Use SSC (Simulator System Call) to
 * display character on debug console.
 *
 * @param c Character to be printed.
 *
 */
static void ski_con_putchar(ski_con_t *con, char ch)
{
	if (ch == '\n')
		ski_con_putchar(con, '\r');

#ifdef UARCH_ia64
	asm volatile (
	    "mov r15 = %0\n"
	    "mov r32 = %1\n"   /* r32 is in0 */
	    "break 0x80000\n"  /* modifies r8 */
	    :
	    : "i" (SKI_PUTCHAR), "r" (ch)
	    : "r15", "in0", "r8"
	);
#else
	(void) ch;
#endif
}

/** Read from Ski console device */
static errno_t ski_con_read(chardev_srv_t *srv, void *buf, size_t size,
    size_t *nread)
{
	ski_con_t *con = (ski_con_t *) srv->srvs->sarg;
	size_t p;
	uint8_t *bp = (uint8_t *) buf;
	errno_t rc;

	fibril_mutex_lock(&con->buf_lock);

	while (circ_buf_nused(&con->cbuf) == 0)
		fibril_condvar_wait(&con->buf_cv, &con->buf_lock);

	p = 0;
	while (p < size) {
		rc = circ_buf_pop(&con->cbuf, &bp[p]);
		if (rc != EOK)
			break;
		++p;
	}

	fibril_mutex_unlock(&con->buf_lock);

	*nread = p;
	return EOK;
}

/** Write to Ski console device */
static errno_t ski_con_write(chardev_srv_t *srv, const void *data, size_t size,
    size_t *nwr)
{
	ski_con_t *con = (ski_con_t *) srv->srvs->sarg;
	size_t i;
	uint8_t *dp = (uint8_t *) data;

	for (i = 0; i < size; i++)
		ski_con_putchar(con, dp[i]);

	*nwr = size;
	return EOK;
}

/** Character device connection handler. */
static void ski_con_connection(cap_call_handle_t icall_handle, ipc_call_t *icall,
    void *arg)
{
	ski_con_t *con = (ski_con_t *) ddf_dev_data_get(
	    ddf_fun_get_dev((ddf_fun_t *) arg));

	chardev_conn(icall_handle, icall, &con->cds);
}

/** @}
 */
