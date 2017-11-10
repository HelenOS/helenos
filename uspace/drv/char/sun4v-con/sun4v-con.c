/*
 * Copyright (c) 2008 Pavel Rimsky
 * Copyright (c) 2011 Jiri Svoboda
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
#include <ipc/char.h>
#include <stdbool.h>
#include <sysinfo.h>
#include <thread.h>

#include "sun4v-con.h"

static void sun4v_con_connection(ipc_callid_t, ipc_call_t *, void *);

#define POLL_INTERVAL  10000

/*
 * Kernel counterpart of the driver pushes characters (it has read) here.
 * Keep in sync with the definition from
 * kernel/arch/sparc64/src/drivers/niagara.c.
 */
#define INPUT_BUFFER_SIZE  ((PAGE_SIZE) - 2 * 8)

typedef volatile struct {
	uint64_t write_ptr;
	uint64_t read_ptr;
	char data[INPUT_BUFFER_SIZE];
} __attribute__((packed)) __attribute__((aligned(PAGE_SIZE))) *input_buffer_t;

/* virtual address of the shared buffer */
static input_buffer_t input_buffer;

static void sun4v_thread_impl(void *arg);

static void sun4v_con_putchar(sun4v_con_t *con, uint8_t data)
{
	(void) con;
	(void) data;
}

/** Add sun4v console device. */
int sun4v_con_add(sun4v_con_t *con)
{
	ddf_fun_t *fun = NULL;
	int rc;

	input_buffer = (input_buffer_t) AS_AREA_ANY;

	fun = ddf_fun_create(con->dev, fun_exposed, "a");
	if (fun == NULL) {
		ddf_msg(LVL_ERROR, "Error creating function 'a'.");
		rc = ENOMEM;
		goto error;
	}

	ddf_fun_set_conn_handler(fun, sun4v_con_connection);

	sysarg_t paddr;
	rc = sysinfo_get_value("niagara.inbuf.address", &paddr);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "niagara.inbuf.address not set (%d)", rc);
		goto error;
	}

	rc = physmem_map(paddr, 1, AS_AREA_READ | AS_AREA_WRITE,
	    (void *) &input_buffer);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error mapping memory: %d", rc);
		goto error;
	}

	thread_id_t tid;
	rc = thread_create(sun4v_thread_impl, con, "kbd_poll", &tid);
	if (rc != EOK)
		goto error;

	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error binding function 'a'.");
		goto error;
	}

	return EOK;
error:
	/* XXX Clean up thread */

	if (input_buffer != (input_buffer_t) AS_AREA_ANY)
		physmem_unmap((void *) input_buffer);

	if (fun != NULL)
		ddf_fun_destroy(fun);

	return rc;
}

/** Remove sun4v console device */
int sun4v_con_remove(sun4v_con_t *con)
{
	return ENOTSUP;
}

/** Msim console device gone */
int sun4v_con_gone(sun4v_con_t *con)
{
	return ENOTSUP;
}

/**
 * Called regularly by the polling thread. Reads codes of all the
 * pressed keys from the buffer.
 */
static void sun4v_key_pressed(sun4v_con_t *con)
{
	char c;

	while (input_buffer->read_ptr != input_buffer->write_ptr) {
		c = input_buffer->data[input_buffer->read_ptr];
		input_buffer->read_ptr =
		    ((input_buffer->read_ptr) + 1) % INPUT_BUFFER_SIZE;
		if (con->client_sess != NULL) {
			async_exch_t *exch = async_exchange_begin(con->client_sess);
			async_msg_1(exch, CHAR_NOTIF_BYTE, c);
			async_exchange_end(exch);
		}
		(void) c;
	}
}

/**
 * Thread to poll Sun4v console for keypresses.
 */
static void sun4v_thread_impl(void *arg)
{
	sun4v_con_t *con = (sun4v_con_t *) arg;

	while (true) {
		sun4v_key_pressed(con);
		thread_usleep(POLL_INTERVAL);
	}
}

/** Character device connection handler. */
static void sun4v_con_connection(ipc_callid_t iid, ipc_call_t *icall,
    void *arg)
{
	sun4v_con_t *con;

	/* Answer the IPC_M_CONNECT_ME_TO call. */
	async_answer_0(iid, EOK);

	con = (sun4v_con_t *)ddf_dev_data_get(ddf_fun_get_dev((ddf_fun_t *)arg));

	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		sysarg_t method = IPC_GET_IMETHOD(call);

		if (!method) {
			/* The other side has hung up. */
			async_answer_0(callid, EOK);
			return;
		}

		async_sess_t *sess =
		    async_callback_receive_start(EXCHANGE_SERIALIZE, &call);
		if (sess != NULL) {
			if (con->client_sess == NULL) {
				con->client_sess = sess;
				async_answer_0(callid, EOK);
			} else
				async_answer_0(callid, ELIMIT);
		} else {
			switch (method) {
			case CHAR_WRITE_BYTE:
				ddf_msg(LVL_DEBUG, "Write %" PRIun " to device\n",
				    IPC_GET_ARG1(call));
				sun4v_con_putchar(con, (uint8_t) IPC_GET_ARG1(call));
				async_answer_0(callid, EOK);
				break;
			default:
				async_answer_0(callid, EINVAL);
			}
		}
	}
}

/** @}
 */
