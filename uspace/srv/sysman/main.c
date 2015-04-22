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
#include <stddef.h>
#include <stdio.h>
#include <str.h>

#include "configuration.h"
#include "dep.h"
#include "job.h"
#include "log.h"
#include "sysman.h"
#include "unit.h"

#define NAME "sysman"

static void sysman_connection(ipc_callid_t callid, ipc_call_t *call, void *arg)
{
	/* TODO handle client connections */
}

/** Build hard coded configuration */
static job_t *create_entry_configuration(void) {
	int result = EOK;
	unit_t *mnt_initrd = NULL;
	unit_t *cfg_init = NULL;
	unit_t *tgt_default = NULL;

	mnt_initrd = unit_create(UNIT_MOUNT);
	if (mnt_initrd == NULL) {
		result = ENOMEM;
		goto fail;
	}
	mnt_initrd->name                 = str_dup("initrd.mnt");
	// TODO Use RDFMT
	CAST_MNT(mnt_initrd)->type       = str_dup("ext4fs");
	CAST_MNT(mnt_initrd)->mountpoint = str_dup("/");
	CAST_MNT(mnt_initrd)->device     = str_dup("bd/initrd");

	cfg_init = unit_create(UNIT_CONFIGURATION);
	if (cfg_init == NULL) {
		result = ENOMEM;
		goto fail;
	}
	cfg_init->name           = str_dup("init.cfg");
	CAST_CFG(cfg_init)->path = str_dup("/cfg/sysman");
	
	tgt_default = unit_create(UNIT_TARGET);
	if (tgt_default == NULL) {
		result = ENOMEM;
		goto fail;
	}
	tgt_default->name = str_dup("default.tgt");
	

	/*
	 * Add units to configuration and start the default target.
	 */
	configuration_start_update();

	configuration_add_unit(mnt_initrd);
	configuration_add_unit(cfg_init);
	configuration_add_unit(tgt_default);

	result = dep_add_dependency(tgt_default, cfg_init);
	if (result != EOK) {
		goto fail;
	}

	result = dep_add_dependency(cfg_init, mnt_initrd);
	if (result != EOK) {
		goto fail;
	}

	configuration_commit();

	job_t *first_job = job_create(tgt_default, STATE_STARTED);
	if (first_job == NULL) {
		goto fail;
	}
	return first_job;

fail:
	// TODO cannot destroy units after they're added to configuration
	unit_destroy(&tgt_default);
	unit_destroy(&cfg_init);
	unit_destroy(&mnt_initrd);
	return NULL;
}

static void first_job_handler(void *object, void *unused)
{
	job_t *job = object;
	sysman_log(LVL_DEBUG, "First job retval: %i.", job->retval);
	job_del_ref(&job);
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
	 * Create initial configuration while we are in a single fibril, keep
	 * the job and run it when event loop is running.
	 */
	job_t *first_job = create_entry_configuration();

	/*
	 * Event loop runs in separate fibril, all consequent access to global
	 * structure is made from this fibril only.
	 */
	fid_t event_loop_fibril = fibril_create(sysman_events_loop, NULL);
	fibril_add_ready(event_loop_fibril);

	/* Queue first job for processing */
	sysman_object_observer(first_job, &first_job_handler, NULL);
	sysman_raise_event(&sysman_event_job_process, first_job);

	/* Start sysman server */
	async_set_client_connection(sysman_connection);

	printf(NAME ": Accepting connections\n");
	async_manager();

	/* not reached */
	return 0;
}
