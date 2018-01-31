/*
 * Copyright (c) 2009 Jakub Jermar
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

/** @addtogroup obio
 * @{
 */

/**
 * @file obio.c
 * @brief OBIO driver.
 *
 * OBIO is a short for on-board I/O. On UltraSPARC IIi and systems with U2P,
 * there is a piece of the root PCI bus controller address space, which
 * contains interrupt mapping and clear registers for all on-board devices.
 * Although UltraSPARC IIi and U2P are different in general, these registers can
 * be found at the same addresses.
 */

#include <async.h>
#include <ddf/driver.h>
#include <ddf/log.h>
#include <ddi.h>
#include <errno.h>
#include <str_error.h>
#include <inttypes.h>
#include <ipc/irc.h>
#include <stdbool.h>
#include <stdio.h>

#include "obio.h"

#define NAME "obio"

#define OBIO_SIZE	0x1898

#define OBIO_IMR_BASE	0x200
#define OBIO_IMR(ino)	(OBIO_IMR_BASE + ((ino) & INO_MASK))

#define OBIO_CIR_BASE	0x300
#define OBIO_CIR(ino)	(OBIO_CIR_BASE + ((ino) & INO_MASK))

#define INO_MASK	0x1f

/** Handle one connection to obio.
 *
 * @param iid		Hash of the request that opened the connection.
 * @param icall		Call data of the request that opened the connection.
 * @param arg		Local argument.
 */
static void obio_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	ipc_callid_t callid;
	ipc_call_t call;
	obio_t *obio;

	/*
	 * Answer the first IPC_M_CONNECT_ME_TO call.
	 */
	async_answer_0(iid, EOK);

	obio = (obio_t *)ddf_dev_data_get(ddf_fun_get_dev((ddf_fun_t *)arg));

	while (1) {
		int inr;

		callid = async_get_call(&call);
		switch (IPC_GET_IMETHOD(call)) {
		case IRC_ENABLE_INTERRUPT:
			inr = IPC_GET_ARG1(call);
			pio_set_64(&obio->regs[OBIO_IMR(inr & INO_MASK)],
			    1UL << 31, 0);
			async_answer_0(callid, EOK);
			break;
		case IRC_DISABLE_INTERRUPT:
			/* XXX TODO */
			async_answer_0(callid, EOK);
			break;
		case IRC_CLEAR_INTERRUPT:
			inr = IPC_GET_ARG1(call);
			pio_write_64(&obio->regs[OBIO_CIR(inr & INO_MASK)], 0);
			async_answer_0(callid, EOK);
			break;
		default:
			async_answer_0(callid, EINVAL);
			break;
		}
	}
}

/** Add OBIO device. */
errno_t obio_add(obio_t *obio, obio_res_t *res)
{
	ddf_fun_t *fun_a = NULL;
	errno_t rc;

	rc = pio_enable((void *)res->base, OBIO_SIZE, (void **) &obio->regs);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error mapping OBIO registers");
		rc = EIO;
		goto error;
	}

	ddf_msg(LVL_NOTE, "OBIO registers with base at 0x%" PRIxn, res->base);

	fun_a = ddf_fun_create(obio->dev, fun_exposed, "a");
	if (fun_a == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating function 'a'.");
		rc = ENOMEM;
		goto error;
	}

	ddf_fun_set_conn_handler(fun_a, obio_connection);

	rc = ddf_fun_bind(fun_a);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function 'a': %s", str_error(rc));
		goto error;
	}

	rc = ddf_fun_add_to_category(fun_a, "irc");
	if (rc != EOK)
		goto error;

	return EOK;
error:
	if (fun_a != NULL)
		ddf_fun_destroy(fun_a);
	return rc;
}

/** Remove OBIO device */
errno_t obio_remove(obio_t *obio)
{
	return ENOTSUP;
}

/** OBIO device gone */
errno_t obio_gone(obio_t *obio)
{
	return ENOTSUP;
}


/**
 * @}
 */
