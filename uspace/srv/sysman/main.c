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

#include "configuration.h"
#include "connection_broker.h"
#include "connection_ctl.h"
#include "dep.h"
#include "job.h"
#include "log.h"
#include "sysman.h"
#include "unit.h"

#define NAME "sysman"

#define INITRD_DEVICE       "bd/initrd"
#define INITRD_MOUNT_POINT  "/"
#define INITRD_CFG_PATH     "/cfg/sysman"

#define TARGET_INIT     "initrd.tgt"
#define TARGET_ROOTFS   "rootfs.tgt"
#define TARGET_DEFAULT  "default.tgt"

#define UNIT_MNT_INITRD "initrd.mnt"
#define UNIT_CFG_INITRD "init.cfg"


static const char *target_sequence[] = {
	TARGET_INIT,
	TARGET_ROOTFS,
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

static void sysman_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	sysman_interface_t iface = IPC_GET_ARG1(*icall);
	switch (iface) {
	case SYSMAN_PORT_BROKER:
		sysman_connection_broker(iid, icall);
		break;
	case SYSMAN_PORT_CTL:
		sysman_connection_ctl(iid, icall);
		break;
	default:
		/* Unknown interface */
		async_answer_0(iid, ENOENT);
	}
}

/** Build hard coded configuration */
static int create_entry_configuration(void) {
	int rc;
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
	configuration_start_update();

	configuration_add_unit(mnt_initrd);
	configuration_add_unit(cfg_init);
	configuration_add_unit(tgt_init);

	rc = dep_add_dependency(tgt_init, cfg_init);
	if (rc != EOK) {
		goto rollback;
	}

	rc = dep_add_dependency(cfg_init, mnt_initrd);
	if (rc != EOK) {
		goto rollback;
	}

	configuration_commit();

	return EOK;

fail:
	unit_destroy(&tgt_init);
	unit_destroy(&cfg_init);
	unit_destroy(&mnt_initrd);
	return rc;

rollback:
	configuration_rollback();
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
	unit_t *tgt = configuration_find_unit_by_name(target_name);
	if (tgt == NULL) {
		sysman_log(LVL_ERROR,
		    "Expected unit '%s' not found in configuration.",
		    target_name);
		return;
	}

	int rc = sysman_run_job(tgt, STATE_STARTED, &sequence_job_handler,
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
	configuration_init();
	sysman_events_init();
	job_queue_init();

	/*
	 * Create initial configuration while we are in a single fibril
	 */
	int rc = create_entry_configuration();
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

	sysman_log(LVL_DEBUG, "Debugging pause...\n");
	async_usleep(10 * 1000000);
	/* Queue first job from sequence */
	prepare_and_run_job(&target_sequence[0]);

	/* We're service too */
	rc = service_register(SERVICE_SYSMAN);
	if (rc != EOK) {
		sysman_log(LVL_FATAL,
		    "Cannot register at naming service (%i).", rc);
		return rc;
	}

	/* Start sysman server */
	async_set_client_connection(sysman_connection);

	printf(NAME ": Accepting connections\n");
	async_manager();

	/* not reached */
	return 0;
}
