/*
 * Copyright (c) 2024 Jiri Svoboda
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

/** @addtogroup taskmon
 * @brief
 * @{
 */
/**
 * @file
 */

#include <async.h>
#include <errno.h>
#include <macros.h>
#include <ipc/services.h>
#include <ipc/corecfg.h>
#include <loc.h>
#include <sif.h>
#include <stdio.h>
#include <str.h>
#include <str_error.h>
#include <task.h>

#define NAME  "taskmon"

static const char *taskmon_cfg_path = "/w/cfg/taskmon.sif";

static bool write_core_files;

static errno_t taskmon_load_cfg(const char *);
static errno_t taskmon_save_cfg(const char *);
static void corecfg_client_conn(ipc_call_t *, void *);

static void fault_event(ipc_call_t *call, void *arg)
{
	const char *fname;
	char *s_taskid;
	char *dump_fname;
	errno_t rc;

	task_id_t taskid;
	uintptr_t thread;

	taskid = MERGE_LOUP32(ipc_get_arg1(call), ipc_get_arg2(call));
	thread = ipc_get_arg3(call);

	if (asprintf(&s_taskid, "%" PRIu64, taskid) < 0) {
		printf("Memory allocation failed.\n");
		return;
	}

	printf(NAME ": Task %" PRIu64 " fault in thread %p.\n", taskid,
	    (void *) thread);

	fname = "/app/taskdump";

	if (write_core_files) {
		if (asprintf(&dump_fname, "/data/core%" PRIu64, taskid) < 0) {
			printf("Memory allocation failed.\n");
			return;
		}

		printf(NAME ": Executing %s -c %s -t %s\n", fname, dump_fname, s_taskid);
		rc = task_spawnl(NULL, NULL, fname, fname, "-c", dump_fname, "-t", s_taskid,
		    NULL);
	} else {
		printf(NAME ": Executing %s -t %s\n", fname, s_taskid);
		rc = task_spawnl(NULL, NULL, fname, fname, "-t", s_taskid, NULL);
	}

	if (rc != EOK) {
		printf("%s: Error spawning %s (%s).\n", NAME, fname,
		    str_error(rc));
	}
}

static void corecfg_get_enable_srv(ipc_call_t *icall)
{
	async_answer_1(icall, EOK, write_core_files);
}

static void corecfg_set_enable_srv(ipc_call_t *icall)
{
	write_core_files = ipc_get_arg1(icall);
	async_answer_0(icall, EOK);
	(void) taskmon_save_cfg(taskmon_cfg_path);
}

static void corecfg_client_conn(ipc_call_t *icall, void *arg)
{
	/* Accept the connection */
	async_accept_0(icall);

	while (true) {
		ipc_call_t call;
		async_get_call(&call);
		sysarg_t method = ipc_get_imethod(&call);

		if (!method) {
			/* The other side has hung up */
			async_answer_0(&call, EOK);
			return;
		}

		switch (method) {
		case CORECFG_GET_ENABLE:
			corecfg_get_enable_srv(&call);
			break;
		case CORECFG_SET_ENABLE:
			corecfg_set_enable_srv(&call);
			break;
		default:
			async_answer_0(&call, ENOTSUP);
		}
	}
}

/** Load task monitor configuration from SIF file.
 *
 * @param cfgpath Configuration file path
 *
 * @return EOK on success or an error code
 */
static errno_t taskmon_load_cfg(const char *cfgpath)
{
	sif_doc_t *doc = NULL;
	sif_node_t *rnode;
	sif_node_t *ncorefiles;
	const char *ntype;
	const char *swrite;
	errno_t rc;

	rc = sif_load(cfgpath, &doc);
	if (rc != EOK)
		goto error;

	rnode = sif_get_root(doc);
	ncorefiles = sif_node_first_child(rnode);
	ntype = sif_node_get_type(ncorefiles);
	if (str_cmp(ntype, "corefiles") != 0) {
		rc = EIO;
		goto error;
	}

	swrite = sif_node_get_attr(ncorefiles, "write");
	if (swrite == NULL) {
		rc = EIO;
		goto error;
	}

	if (str_cmp(swrite, "y") == 0) {
		write_core_files = true;
	} else if (str_cmp(swrite, "n") == 0) {
		write_core_files = false;
	} else {
		rc = EIO;
		goto error;
	}

	sif_delete(doc);
	return EOK;
error:
	if (doc != NULL)
		sif_delete(doc);
	return rc;
}

/** Save task monitor configuration to SIF file.
 *
 * @param cfgpath Configuration file path
 *
 * @return EOK on success or an error code
 */
static errno_t taskmon_save_cfg(const char *cfgpath)
{
	sif_doc_t *doc = NULL;
	sif_node_t *rnode;
	sif_node_t *ncorefiles;
	errno_t rc;

	rc = sif_new(&doc);
	if (rc != EOK)
		goto error;

	rnode = sif_get_root(doc);
	rc = sif_node_append_child(rnode, "corefiles", &ncorefiles);
	if (rc != EOK)
		goto error;

	rc = sif_node_set_attr(ncorefiles, "write",
	    write_core_files ? "y" : "n");
	if (rc != EOK)
		goto error;

	rc = sif_save(doc, cfgpath);
	if (rc != EOK)
		goto error;

	sif_delete(doc);
	return EOK;
error:
	if (doc != NULL)
		sif_delete(doc);
	return rc;
}

int main(int argc, char *argv[])
{
	loc_srv_t *srv;

	printf("%s: Task Monitoring Service\n", NAME);

#ifdef CONFIG_WRITE_CORE_FILES
	write_core_files = true;
#else
	write_core_files = false;
#endif
	(void) taskmon_load_cfg(taskmon_cfg_path);

	if (async_event_subscribe(EVENT_FAULT, fault_event, NULL) != EOK) {
		printf("%s: Error registering fault notifications.\n", NAME);
		return -1;
	}

	async_set_fallback_port_handler(corecfg_client_conn, NULL);

	errno_t rc = loc_server_register(NAME, &srv);
	if (rc != EOK) {
		printf("%s: Failed registering server: %s.\n",
		    NAME, str_error(rc));
		return -1;
	}

	service_id_t sid;
	rc = loc_service_register(srv, SERVICE_NAME_CORECFG, &sid);
	if (rc != EOK) {
		loc_server_unregister(srv);
		printf("%s: Failed registering service: %s.\n",
		    NAME, str_error(rc));
		return -1;
	}

	task_retval(0);
	async_manager();

	return 0;
}

/** @}
 */
