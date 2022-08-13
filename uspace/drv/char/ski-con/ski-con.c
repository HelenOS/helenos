/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file Ski console driver.
 */

#include <as.h>
#include <async.h>
#include <ddf/driver.h>
#include <ddf/log.h>
#include <ddi.h>
#include <errno.h>
#include <fibril.h>
#include <io/chardev.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sysinfo.h>

#include "ski-con.h"

#define SKI_GETCHAR		21
#define SKI_PUTCHAR		31

#define POLL_INTERVAL		10000

static errno_t ski_con_fibril(void *arg);
static int32_t ski_con_getchar(void);
static void ski_con_connection(ipc_call_t *, void *);

static errno_t ski_con_read(chardev_srv_t *, void *, size_t, size_t *,
    chardev_flags_t);
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
	uintptr_t faddr;
	void *addr = AS_AREA_ANY;
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

	rc = sysinfo_get_value("ski.paddr", &faddr);
	if (rc != EOK)
		faddr = 0; /* No kernel driver to arbitrate with */

	if (faddr != 0) {
		addr = AS_AREA_ANY;
		rc = physmem_map(faddr, 1, AS_AREA_READ | AS_AREA_CACHEABLE,
		    &addr);
		if (rc != EOK) {
			ddf_msg(LVL_ERROR, "Cannot map kernel driver arbitration area.");
			goto error;
		}
	}

	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error binding function 'a'.");
		goto error;
	}

	bound = true;

	rc = ddf_fun_add_to_category(fun, "console");
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error adding function 'a' to category "
		    "'console'.");
		goto error;
	}

	fid = fibril_create(ski_con_fibril, con);
	if (fid == 0) {
		ddf_msg(LVL_ERROR, "Error creating fibril.");
		rc = ENOMEM;
		goto error;
	}

	fibril_add_ready(fid);
	return EOK;
error:
	if (addr != AS_AREA_ANY)
		as_area_destroy(addr);
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

/** Detect if SKI console is in use by the kernel.
 *
 * This is needed since the kernel has no way of fencing off the user-space
 * driver.
 *
 * @return @c true if in use by the kernel.
 */
static bool ski_con_disabled(void)
{
	sysarg_t kconsole;

	/*
	 * XXX Ideally we should get information from our kernel counterpart
	 * driver. But there needs to be a mechanism for the kernel console
	 * to inform the kernel driver.
	 */
	if (sysinfo_get_value("kconsole", &kconsole) != EOK)
		return false;

	return kconsole != false;
}

/** Poll Ski for keypresses. */
static errno_t ski_con_fibril(void *arg)
{
	int32_t c;
	ski_con_t *con = (ski_con_t *) arg;
	errno_t rc;

	while (true) {
		while (!ski_con_disabled()) {
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

		fibril_usleep(POLL_INTERVAL);
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
    size_t *nread, chardev_flags_t flags)
{
	ski_con_t *con = (ski_con_t *) srv->srvs->sarg;
	size_t p;
	uint8_t *bp = (uint8_t *) buf;
	errno_t rc;

	fibril_mutex_lock(&con->buf_lock);

	while ((flags & chardev_f_nonblock) == 0 &&
	    circ_buf_nused(&con->cbuf) == 0)
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

	if (!ski_con_disabled()) {
		for (i = 0; i < size; i++) {
			ski_con_putchar(con, dp[i]);
		}
	}

	*nwr = size;
	return EOK;
}

/** Character device connection handler. */
static void ski_con_connection(ipc_call_t *icall, void *arg)
{
	ski_con_t *con = (ski_con_t *) ddf_dev_data_get(
	    ddf_fun_get_dev((ddf_fun_t *) arg));

	chardev_conn(icall, &con->cds);
}

/** @}
 */
