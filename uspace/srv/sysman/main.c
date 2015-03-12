#include <async.h>
#include <errno.h>
#include <fibril.h>
#include <stddef.h>
#include <stdio.h>

#include "configuration.h"
#include "dep.h"
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
	// TODO Use RDFMT
	mnt_initrd->data.mnt.type       = "ext4fs";
	mnt_initrd->data.mnt.mountpoint = "/";
	mnt_initrd->data.mnt.device     = "bd/initrd";

	cfg_init = unit_create(UNIT_CONFIGURATION);
	if (cfg_init == NULL) {
		result = ENOMEM;
		goto fail;
	}
	cfg_init->data.cfg.path = "/cfg/";
	
	tgt_default = unit_create(UNIT_TARGET);
	if (tgt_default == NULL) {
		result = ENOMEM;
		goto fail;
	}
	

	/*
	 * Add units to configuration and start the default target.
	 */
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
