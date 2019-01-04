/*
 * Copyright (c) 2008 Pavel Rimsky
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

/** @file Sun4v console driver
 */

#include <as.h>
#include <async.h>
#include <ddf/log.h>
#include <ddi.h>
#include <errno.h>
#include <str_error.h>
#include <io/chardev_srv.h>
#include <stdbool.h>

#include "sun4v-con.h"

static void sun4v_con_connection(ipc_call_t *, void *);

#define POLL_INTERVAL  10000

static errno_t sun4v_con_read(chardev_srv_t *, void *, size_t, size_t *,
    chardev_flags_t);
static errno_t sun4v_con_write(chardev_srv_t *, const void *, size_t, size_t *);

static chardev_ops_t sun4v_con_chardev_ops = {
	.read = sun4v_con_read,
	.write = sun4v_con_write
};

static void sun4v_con_putchar(sun4v_con_t *con, uint8_t data)
{
	if (data == '\n')
		sun4v_con_putchar(con, '\r');

	while (con->output_buffer->write_ptr ==
	    (con->output_buffer->read_ptr + OUTPUT_BUFFER_SIZE - 1) %
	    OUTPUT_BUFFER_SIZE)
		;

	con->output_buffer->data[con->output_buffer->write_ptr] = data;
	con->output_buffer->write_ptr =
	    ((con->output_buffer->write_ptr) + 1) % OUTPUT_BUFFER_SIZE;
}

/** Add sun4v console device. */
errno_t sun4v_con_add(sun4v_con_t *con, sun4v_con_res_t *res)
{
	ddf_fun_t *fun = NULL;
	errno_t rc;
	bool bound = false;

	con->res = *res;
	con->input_buffer = (niagara_input_buffer_t *) AS_AREA_ANY;

	fun = ddf_fun_create(con->dev, fun_exposed, "a");
	if (fun == NULL) {
		ddf_msg(LVL_ERROR, "Error creating function 'a'.");
		rc = ENOMEM;
		goto error;
	}

	chardev_srvs_init(&con->cds);
	con->cds.ops = &sun4v_con_chardev_ops;
	con->cds.sarg = con;

	ddf_fun_set_conn_handler(fun, sun4v_con_connection);

	rc = physmem_map(res->in_base, 1, AS_AREA_READ | AS_AREA_WRITE,
	    (void *) &con->input_buffer);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error mapping memory: %s", str_error_name(rc));
		goto error;
	}

	con->output_buffer = (niagara_output_buffer_t *) AS_AREA_ANY;

	rc = physmem_map(res->out_base, 1, AS_AREA_READ | AS_AREA_WRITE,
	    (void *) &con->output_buffer);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error mapping memory: %s", str_error_name(rc));
		return rc;
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

	return EOK;
error:
	if (con->input_buffer != (niagara_input_buffer_t *) AS_AREA_ANY)
		physmem_unmap((void *) con->input_buffer);

	if (con->output_buffer != (niagara_output_buffer_t *) AS_AREA_ANY)
		physmem_unmap((void *) con->output_buffer);

	if (bound)
		ddf_fun_unbind(fun);

	if (fun != NULL)
		ddf_fun_destroy(fun);

	return rc;
}

/** Remove sun4v console device */
errno_t sun4v_con_remove(sun4v_con_t *con)
{
	return ENOTSUP;
}

/** Msim console device gone */
errno_t sun4v_con_gone(sun4v_con_t *con)
{
	return ENOTSUP;
}

/** Read from Sun4v console device */
static errno_t sun4v_con_read(chardev_srv_t *srv, void *buf, size_t size,
    size_t *nread, chardev_flags_t flags)
{
	sun4v_con_t *con = (sun4v_con_t *) srv->srvs->sarg;
	size_t p;
	uint8_t *bp = (uint8_t *) buf;
	char c;

	while ((flags & chardev_f_nonblock) == 0 &&
	    con->input_buffer->read_ptr == con->input_buffer->write_ptr)
		fibril_usleep(POLL_INTERVAL);

	p = 0;
	while (p < size && con->input_buffer->read_ptr != con->input_buffer->write_ptr) {
		c = con->input_buffer->data[con->input_buffer->read_ptr];
		con->input_buffer->read_ptr =
		    ((con->input_buffer->read_ptr) + 1) % INPUT_BUFFER_SIZE;
		bp[p++] = c;
	}

	*nread = p;
	return EOK;
}

/** Write to Sun4v console device */
static errno_t sun4v_con_write(chardev_srv_t *srv, const void *data, size_t size,
    size_t *nwr)
{
	sun4v_con_t *con = (sun4v_con_t *) srv->srvs->sarg;
	size_t i;
	uint8_t *dp = (uint8_t *) data;

	for (i = 0; i < size; i++)
		sun4v_con_putchar(con, dp[i]);

	*nwr = size;
	return EOK;
}

/** Character device connection handler. */
static void sun4v_con_connection(ipc_call_t *icall, void *arg)
{
	sun4v_con_t *con = (sun4v_con_t *) ddf_dev_data_get(
	    ddf_fun_get_dev((ddf_fun_t *) arg));

	chardev_conn(icall, &con->cds);
}

/** @}
 */
