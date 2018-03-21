/*
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

/** @addtogroup amdm37x
 * @{
 */
/**
 * @file
 */

#include <ddf/driver.h>
#include <ddf/log.h>
#include <errno.h>
#include <str_error.h>
#include <stdio.h>
#include <graph.h>

#include "amdm37x_dispc.h"

#define NAME  "amdm37x_dispc"

static void graph_vsl_connection(cap_call_handle_t icall_handle, ipc_call_t *icall, void *arg)
{
	visualizer_t *vsl;

	vsl = (visualizer_t *) ddf_fun_data_get((ddf_fun_t *)arg);
	graph_visualizer_connection(vsl, icall_handle, icall, NULL);
}

static errno_t amdm37x_dispc_dev_add(ddf_dev_t *dev)
{
	assert(dev);
	/* Visualizer part */
	ddf_fun_t *fun = ddf_fun_create(dev, fun_exposed, "viz");
	if (!fun) {
		ddf_log_error("Failed to create visualizer function\n");
		return ENOMEM;
	}

	visualizer_t *vis = ddf_fun_data_alloc(fun, sizeof(visualizer_t));
	if (!vis) {
		ddf_log_error("Failed to allocate visualizer structure\n");
		ddf_fun_destroy(fun);
		return ENOMEM;
	}

	graph_init_visualizer(vis);
	vis->reg_svc_handle = ddf_fun_get_handle(fun);

	ddf_fun_set_conn_handler(fun, graph_vsl_connection);
	/* Hw part */
	amdm37x_dispc_t *dispc =
	    ddf_dev_data_alloc(dev, sizeof(amdm37x_dispc_t));
	if (!dispc) {
		ddf_log_error("Failed to allocate dispc structure\n");
		ddf_fun_destroy(fun);
		return ENOMEM;
	}

	errno_t ret = amdm37x_dispc_init(dispc, vis);
	if (ret != EOK) {
		ddf_log_error("Failed to init dispc: %s\n", str_error(ret));
		ddf_fun_destroy(fun);
		return ret;
	}

	/* Bind function */
	ret = ddf_fun_bind(fun);
	if (ret != EOK) {
		ddf_log_error("Failed to bind function: %s\n", str_error(ret));
		amdm37x_dispc_fini(dispc);
		ddf_fun_destroy(fun);
		return ret;
	}
	ddf_fun_add_to_category(fun, "visualizer");

	ddf_log_note("Added device `%s'\n", ddf_dev_get_name(dev));
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
