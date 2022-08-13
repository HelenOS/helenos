/*
 * SPDX-FileCopyrightText: 2006 Josef Cejka
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 * @brief Msim console driver.
 */

#include <async.h>
#include <ddf/driver.h>
#include <ddf/log.h>
#include <ddi.h>
#include <errno.h>
#include <io/chardev_srv.h>

#include "msim-con.h"

static void msim_con_connection(ipc_call_t *, void *);

static errno_t msim_con_read(chardev_srv_t *, void *, size_t, size_t *,
    chardev_flags_t);
static errno_t msim_con_write(chardev_srv_t *, const void *, size_t, size_t *);

static chardev_ops_t msim_con_chardev_ops = {
	.read = msim_con_read,
	.write = msim_con_write
};

static irq_cmd_t msim_cmds_proto[] = {
	{
		.cmd = CMD_PIO_READ_8,
		.addr = (void *) 0,	/* will be patched in run-time */
		.dstarg = 2
	},
	{
		.cmd = CMD_ACCEPT
	}
};

static void msim_irq_handler(ipc_call_t *call, void *arg)
{
	msim_con_t *con = (msim_con_t *) arg;
	uint8_t c;
	errno_t rc;

	fibril_mutex_lock(&con->buf_lock);

	c = ipc_get_arg2(call);
	rc = circ_buf_push(&con->cbuf, &c);
	if (rc != EOK)
		ddf_msg(LVL_ERROR, "Buffer overrun");

	fibril_mutex_unlock(&con->buf_lock);
	fibril_condvar_broadcast(&con->buf_cv);
}

/** Add msim console device. */
errno_t msim_con_add(msim_con_t *con, msim_con_res_t *res)
{
	ddf_fun_t *fun = NULL;
	irq_cmd_t *msim_cmds = NULL;
	errno_t rc;
	bool bound = false;

	circ_buf_init(&con->cbuf, con->buf, msim_con_buf_size, 1);
	fibril_mutex_initialize(&con->buf_lock);
	fibril_condvar_initialize(&con->buf_cv);

	con->irq_handle = CAP_NIL;

	msim_cmds = malloc(sizeof(msim_cmds_proto));
	if (msim_cmds == NULL) {
		rc = ENOMEM;
		goto error;
	}

	con->res = *res;

	fun = ddf_fun_create(con->dev, fun_exposed, "a");
	if (fun == NULL) {
		ddf_msg(LVL_ERROR, "Error creating function 'a'.");
		rc = ENOMEM;
		goto error;
	}

	rc = pio_enable((void *)res->base, 1, (void **) &con->out_reg);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error enabling I/O");
		goto error;
	}

	ddf_fun_set_conn_handler(fun, msim_con_connection);

	con->irq_range[0].base = res->base;
	con->irq_range[0].size = 1;

	memcpy(msim_cmds, msim_cmds_proto, sizeof(msim_cmds_proto));
	msim_cmds[0].addr = (void *) res->base;

	con->irq_code.rangecount = 1;
	con->irq_code.ranges = con->irq_range;
	con->irq_code.cmdcount = sizeof(msim_cmds_proto) / sizeof(irq_cmd_t);
	con->irq_code.cmds = msim_cmds;

	rc = async_irq_subscribe(res->irq, msim_irq_handler, con,
	    &con->irq_code, &con->irq_handle);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error registering IRQ code.");
		goto error;
	}

	chardev_srvs_init(&con->cds);
	con->cds.ops = &msim_con_chardev_ops;
	con->cds.sarg = con;

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

	return EOK;
error:
	if (cap_handle_valid(con->irq_handle))
		async_irq_unsubscribe(con->irq_handle);
	if (bound)
		ddf_fun_unbind(fun);
	if (fun != NULL)
		ddf_fun_destroy(fun);
	free(msim_cmds);

	return rc;
}

/** Remove msim console device */
errno_t msim_con_remove(msim_con_t *con)
{
	return ENOTSUP;
}

/** Msim console device gone */
errno_t msim_con_gone(msim_con_t *con)
{
	return ENOTSUP;
}

static void msim_con_putchar(msim_con_t *con, uint8_t ch)
{
	pio_write_8(con->out_reg, ch);
}

/** Read from msim console device */
static errno_t msim_con_read(chardev_srv_t *srv, void *buf, size_t size,
    size_t *nread, chardev_flags_t flags)
{
	msim_con_t *con = (msim_con_t *) srv->srvs->sarg;
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

/** Write to msim console device */
static errno_t msim_con_write(chardev_srv_t *srv, const void *data, size_t size,
    size_t *nwr)
{
	msim_con_t *con = (msim_con_t *) srv->srvs->sarg;
	size_t i;
	uint8_t *dp = (uint8_t *) data;

	for (i = 0; i < size; i++)
		msim_con_putchar(con, dp[i]);

	*nwr = size;
	return EOK;
}

/** Character device connection handler. */
static void msim_con_connection(ipc_call_t *icall, void *arg)
{
	msim_con_t *con = (msim_con_t *) ddf_dev_data_get(
	    ddf_fun_get_dev((ddf_fun_t *) arg));

	chardev_conn(icall, &con->cds);
}

/** @}
 */
