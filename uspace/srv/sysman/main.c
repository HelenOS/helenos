#include <async.h>
#include <errno.h>
#include <fibril.h>
#include <stddef.h>
#include <stdio.h>
#include <str.h>

#include "configuration.h"
#include "dep.h"
#include "job.h"
#include "sysman.h"
#include "unit.h"

#define NAME "sysman"

static void sysman_connection(ipc_callid_t callid, ipc_call_t *call, void *arg)
{
	/* TODO handle client connections */
}

static int sysman_entry_point(void *arg) {
	/*
	 * Build hard coded configuration.
	 *
	 * Strings are allocated on heap, so that they can be free'd by an
	 * owning unit.
	 */
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

	result = sysman_unit_start(tgt_default);

	return result;

fail:
	unit_destroy(&tgt_default);
	unit_destroy(&cfg_init);
	unit_destroy(&mnt_initrd);
	return result;
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS system daemon\n");

	configuration_init();
	job_queue_init();

	/*
	 * Create and start initial configuration asynchronously
	 * so that we can start server's fibril that may be used
	 * when executing the start.
	 */
	fid_t entry_fibril = fibril_create(sysman_entry_point, NULL);
	fibril_add_ready(entry_fibril);

	/* Prepare and start sysman server */
	async_set_client_connection(sysman_connection);

	printf(NAME ": Accepting connections\n");
	async_manager();

	return 0;
}
