/*
 * Copyright (c) 2015 Michal Koutny
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AS IS'' AND ANY EXPRESS OR
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

#include <async.h>
#include <errno.h>
#include <fibril.h>
#include <ipc/sysman.h>
#include <macros.h>
#include <ns.h>
#include <stddef.h>
#include <stdio.h>
#include <str.h>
#include <taskman.h>

#include "connection_broker.h"
#include "connection_ctl.h"
#include "edge.h"
#include "job_queue.h"
#include "log.h"
#include "repo.h"
#include "sm_task.h"
#include "sysman.h"
#include "unit.h"

#define NAME "sysman"

static const char *target_sequence[] = {
	TARGET_INIT,
	//TODO: Mount root fs
	//TARGET_ROOTFS,
	TARGET_DEFAULT,
	NULL
};

/*
 * Forward declarations
 */
static void prepare_and_run_job(const char **target_name_ptr);

/*
 * Static functions
 */

static void sysman_connection(ipc_call_t *icall, void *arg)
{
	/* First, accept connection */
	async_accept_0(icall);

	sysman_interface_t iface = ipc_get_arg2(icall);
	switch (iface) {
	case SYSMAN_PORT_BROKER:
		sysman_connection_broker(icall);
		break;
	case SYSMAN_PORT_CTL:
		sysman_connection_ctl(icall);
		break;
	default:
		/* Unknown interface */
		async_answer_0(icall, ENOENT);
	}
}

/** Build hard coded configuration */
static errno_t create_entry_configuration(void)
{
	errno_t rc;
	unit_t *mnt_initrd = NULL;
	unit_t *cfg_init = NULL;
	unit_t *tgt_init = NULL;

	mnt_initrd = unit_create(UNIT_MOUNT);
	if (mnt_initrd == NULL) {
		rc = ENOMEM;
		goto fail;
	}
	mnt_initrd->name                 = str_dup(UNIT_MNT_INITRD);
	CAST_MNT(mnt_initrd)->type       = str_dup(STRING(RDFMT));
	CAST_MNT(mnt_initrd)->mountpoint = str_dup(INITRD_MOUNT_POINT);
	CAST_MNT(mnt_initrd)->device     = str_dup(INITRD_DEVICE);
	CAST_MNT(mnt_initrd)->autostart  = false;
	CAST_MNT(mnt_initrd)->blocking   = true;

	cfg_init = unit_create(UNIT_CONFIGURATION);
	if (cfg_init == NULL) {
		rc = ENOMEM;
		goto fail;
	}
	cfg_init->name           = str_dup(UNIT_CFG_INITRD);
	CAST_CFG(cfg_init)->path = str_dup(INITRD_CFG_PATH);

	tgt_init = unit_create(UNIT_TARGET);
	if (tgt_init == NULL) {
		rc = ENOMEM;
		goto fail;
	}
	tgt_init->name = str_dup(TARGET_INIT);

	/*
	 * Add units to configuration and start the default target.
	 */
	repo_begin_update();

	rc = repo_add_unit(mnt_initrd);
	if (rc != EOK)
		goto rollback;
	rc = repo_add_unit(cfg_init);
	if (rc != EOK)
		goto rollback;
	rc = repo_add_unit(tgt_init);
	if (rc != EOK)
		goto rollback;

	rc = edge_connect(tgt_init, cfg_init);
	if (rc != EOK)
		goto rollback;

	rc = edge_connect(cfg_init, mnt_initrd);
	if (rc != EOK)
		goto rollback;

	repo_commit();
	return EOK;

fail:
	unit_destroy(&tgt_init);
	unit_destroy(&cfg_init);
	unit_destroy(&mnt_initrd);
	return rc;

rollback:
	repo_rollback();
	return rc;
}

static void sequence_job_handler(void *object, void *arg)
{
	job_t *job = object;
	if (job->retval == JOB_FAILED) {
		sysman_log(LVL_ERROR, "Failed to start '%s'.", unit_name(job->unit));
		job_del_ref(&job);
		return;
	}
	job_del_ref(&job);

	const char **target_name_ptr = arg;
	prepare_and_run_job(target_name_ptr + 1);
}

static void prepare_and_run_job(const char **target_name_ptr)
{
	const char *target_name = *target_name_ptr;

	if (target_name == NULL) {
		sysman_log(LVL_NOTE, "All initial units started.");
		return;
	}

	/* Previous targets should have loaded new units */
	unit_t *tgt = repo_find_unit_by_name(target_name);
	if (tgt == NULL) {
		sysman_log(LVL_ERROR,
		    "Expected unit '%s' not found in configuration.",
		    target_name);
		return;
	}

	errno_t rc = sysman_run_job(tgt, STATE_STARTED, 0, &sequence_job_handler,
	    target_name_ptr);

	if (rc != EOK) {
		sysman_log(LVL_FATAL, "Cannot create job for '%s'.", target_name);
	}
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS system daemon\n");

	/*
	 * Initialize global structures
	 */
	// TODO check return values and abort start
	repo_init();
	sysman_events_init();
	job_queue_init();

	/*
	 * Create initial configuration while we are in a single fibril
	 */
	errno_t rc = create_entry_configuration();
	if (rc != EOK) {
		sysman_log(LVL_FATAL,
		    "Could not create initial configuration (%i).", rc);
		return rc;
	}

	/*
	 * Event loop runs in separate fibril, all consequent access to global
	 * structure is made from this fibril only.
	 */
	fid_t event_loop_fibril = fibril_create(sysman_events_loop, NULL);
	fibril_add_ready(event_loop_fibril);

	/* We're service too */
	rc = service_register(SERVICE_SYSMAN, INTERFACE_SYSMAN, sysman_connection, NULL);
	if (rc != EOK) {
		sysman_log(LVL_FATAL,
		    "Cannot register at naming service (%i).", rc);
		return rc;
	}

	/* Start listening task events and scan boot time tasks */
	rc = sm_task_start();
	if (rc != EOK) {
		sysman_log(LVL_FATAL,
		    "Cannot scan boot time tasks (%i).", rc);
		return rc;
	}

	/* Queue first job from sequence */
	prepare_and_run_job(&target_sequence[0]);

	/* Start sysman server */

	printf(NAME ": Accepting connections\n");
	task_retval(0);
	async_manager();

	/* not reached */
	return 0;
}
