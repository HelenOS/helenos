/*
 * Copyright (c) 2020 Jiri Svoboda
 * Copyright (c) 2013 Jan Vesely
 * Copyright (c) 2011 Petr Koupy
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

/** @addtogroup amdm37x_dispc
 * @{
 */
/**
 * @file
 */

#include <ddev_srv.h>
#include <ddf/driver.h>
#include <ddf/log.h>
#include <errno.h>
#include <ipcgfx/server.h>
#include <str_error.h>
#include <stdio.h>

#include "amdm37x_dispc.h"

#define NAME  "amdm37x_dispc"

static void amdm37x_client_conn(ipc_call_t *icall, void *arg)
{
	amdm37x_dispc_t *dispc;
	ddev_srv_t srv;
	sysarg_t gc_id;
	gfx_context_t *gc;
	errno_t rc;

	dispc = (amdm37x_dispc_t *) ddf_dev_data_get(
	    ddf_fun_get_dev((ddf_fun_t *) arg));

	gc_id = ipc_get_arg3(icall);

	if (gc_id == 0) {
		/* Set up protocol structure */
		ddev_srv_initialize(&srv);
		srv.ops = &amdm37x_ddev_ops;
		srv.arg = dispc;

		/* Handle connection */
		ddev_conn(icall, &srv);
	} else {
		assert(gc_id == 42);

		rc = gfx_context_new(&amdm37x_gc_ops, dispc, &gc);
		if (rc != EOK)
			goto error;

		/* GC connection */
		gc_conn(icall, gc);
	}

	return;
error:
	async_answer_0(icall, rc);
}

static errno_t amdm37x_dispc_dev_add(ddf_dev_t *dev)
{
	assert(dev);

	ddf_fun_t *fun = ddf_fun_create(dev, fun_exposed, "a");
	if (!fun) {
		ddf_log_error("Failed to create display device function.");
		return ENOMEM;
	}

	ddf_fun_set_conn_handler(fun, &amdm37x_client_conn);

	/* Hw part */
	amdm37x_dispc_t *dispc =
	    ddf_dev_data_alloc(dev, sizeof(amdm37x_dispc_t));
	if (!dispc) {
		ddf_log_error("Failed to allocate dispc structure.");
		ddf_fun_destroy(fun);
		return ENOMEM;
	}

	errno_t rc = amdm37x_dispc_init(dispc, fun);
	if (rc != EOK) {
		ddf_log_error("Failed to init dispc: %s.", str_error(rc));
		ddf_fun_destroy(fun);
		return rc;
	}

	/* Bind function */
	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_log_error("Failed to bind function: %s.", str_error(rc));
		amdm37x_dispc_fini(dispc);
		ddf_fun_destroy(fun);
		return rc;
	}

	rc = ddf_fun_add_to_category(fun, "display-device");
	if (rc != EOK) {
		ddf_log_error("Failed to add function: %s to display device "
		    "category.", str_error(rc));
		amdm37x_dispc_fini(dispc);
		ddf_fun_unbind(fun);
		ddf_fun_destroy(fun);
		return rc;
	}

	ddf_log_note("Added device `%s'", ddf_dev_get_name(dev));
	return EOK;
}

static driver_ops_t amdm37x_dispc_driver_ops = {
	.dev_add = amdm37x_dispc_dev_add,
};

static driver_t amdm37x_dispc_driver = {
	.name = NAME,
	.driver_ops = &amdm37x_dispc_driver_ops
};

int main(int argc, char *argv[])
{
	printf("%s: HelenOS AM/DM37x framebuffer driver\n", NAME);
	ddf_log_init(NAME);
	return ddf_driver_main(&amdm37x_dispc_driver);
}

/** @}
 */
