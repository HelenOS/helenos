/*
 * Copyright (c) 2025 Jiri Svoboda
 * Copyright (c) 2005 Martin Decky
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

/** @addtogroup init
 * @{
 */
/**
 * @file
 */

#include <devman.h>
#include <fibril.h>
#include <futil.h>
#include <io/log.h>
#include <stdio.h>
#include <stdarg.h>
#include <vfs/vfs.h>
#include <stdbool.h>
#include <errno.h>
#include <task.h>
#include <stdlib.h>
#include <macros.h>
#include <str.h>
#include <loc.h>
#include <shutdown.h>
#include <str_error.h>
#include <config.h>
#include <io/logctl.h>
#include <vfs/vfs.h>
#include <vol.h>
#include <system.h>
#include <system_srv.h>
#include "system.h"

#define BANNER_LEFT   "######> "
#define BANNER_RIGHT  " <######"

#define LOCFS_FS_TYPE      "locfs"
#define LOCFS_MOUNT_POINT  "/loc"

#define TMPFS_FS_TYPE      "tmpfs"
#define TMPFS_MOUNT_POINT  "/tmp"

#define SRV_CONSOLE  "/srv/hid/console"
#define APP_GETTERM  "/app/getterm"

#define SRV_DISPLAY  "/srv/hid/display"

#define HID_INPUT              "hid/input"
#define HID_OUTPUT             "hid/output"

#define srv_start(path, ...) \
	srv_startl(path, path, ##__VA_ARGS__, NULL)

static const char *sys_dirs[] = {
	"/w/cfg",
	"/w/data",
	NULL,
};

static void system_srv_conn(ipc_call_t *, void *);
static errno_t system_srv_poweroff(void *);
static errno_t system_srv_restart(void *);

system_ops_t system_srv_ops = {
	.poweroff = system_srv_poweroff,
	.restart = system_srv_restart
};

/** Print banner */
static void info_print(void)
{
	printf("%s: HelenOS system server\n", NAME);
}

static void oom_check(errno_t rc, const char *path)
{
	if (rc == ENOMEM) {
		printf("%sOut-of-memory condition detected%s\n", BANNER_LEFT,
		    BANNER_RIGHT);
		printf("%sBailing out of the boot process after %s%s\n",
		    BANNER_LEFT, path, BANNER_RIGHT);
		printf("%sMore physical memory is required%s\n", BANNER_LEFT,
		    BANNER_RIGHT);
		exit(ENOMEM);
	}
}

/** Report mount operation success */
static bool mount_report(const char *desc, const char *mntpt,
    const char *fstype, const char *dev, errno_t rc)
{
	switch (rc) {
	case EOK:
		if ((dev != NULL) && (str_cmp(dev, "") != 0))
			printf("%s: %s mounted on %s (%s at %s)\n", NAME, desc, mntpt,
			    fstype, dev);
		else
			printf("%s: %s mounted on %s (%s)\n", NAME, desc, mntpt, fstype);
		break;
	case EBUSY:
		printf("%s: %s already mounted on %s\n", NAME, desc, mntpt);
		return false;
	case ELIMIT:
		printf("%s: %s limit exceeded\n", NAME, desc);
		return false;
	case ENOENT:
		printf("%s: %s unknown type (%s)\n", NAME, desc, fstype);
		return false;
	default:
		printf("%s: %s not mounted on %s (%s)\n", NAME, desc, mntpt,
		    str_error(rc));
		return false;
	}

	return true;
}

/** Mount locfs file system
 *
 * The operation blocks until the locfs file system
 * server is ready for mounting.
 *
 * @return True on success.
 * @return False on failure.
 *
 */
static bool mount_locfs(void)
{
	errno_t rc = vfs_mount_path(LOCFS_MOUNT_POINT, LOCFS_FS_TYPE, "", "",
	    IPC_FLAG_BLOCKING, 0);
	return mount_report("Location service file system", LOCFS_MOUNT_POINT,
	    LOCFS_FS_TYPE, NULL, rc);
}

static errno_t srv_startl(const char *path, ...)
{
	vfs_stat_t s;
	if (vfs_stat_path(path, &s) != EOK) {
		printf("%s: Unable to stat %s\n", NAME, path);
		return ENOENT;
	}

	printf("%s: Starting %s\n", NAME, path);

	va_list ap;
	const char *arg;
	int cnt = 0;

	va_start(ap, path);
	do {
		arg = va_arg(ap, const char *);
		cnt++;
	} while (arg != NULL);
	va_end(ap);

	va_start(ap, path);
	task_id_t id;
	task_wait_t wait;
	errno_t rc = task_spawn(&id, &wait, path, cnt, ap);
	va_end(ap);

	if (rc != EOK) {
		oom_check(rc, path);
		printf("%s: Error spawning %s (%s)\n", NAME, path,
		    str_error(rc));
		return rc;
	}

	if (!id) {
		printf("%s: Error spawning %s (invalid task id)\n", NAME,
		    path);
		return EINVAL;
	}

	task_exit_t texit;
	int retval;
	rc = task_wait(&wait, &texit, &retval);
	if (rc != EOK) {
		printf("%s: Error waiting for %s (%s)\n", NAME, path,
		    str_error(rc));
		return rc;
	}

	if (texit != TASK_EXIT_NORMAL) {
		printf("%s: Server %s failed to start (unexpectedly "
		    "terminated)\n", NAME, path);
		return EINVAL;
	}

	if (retval != 0)
		printf("%s: Server %s failed to start (exit code %d)\n", NAME,
		    path, retval);

	return retval == 0 ? EOK : EPARTY;
}

static errno_t console(const char *isvc, const char *osvc)
{
	/* Wait for the input service to be ready */
	service_id_t service_id;
	errno_t rc = loc_service_get_id(isvc, &service_id, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		printf("%s: Error waiting on %s (%s)\n", NAME, isvc,
		    str_error(rc));
		return rc;
	}

	/* Wait for the output service to be ready */
	rc = loc_service_get_id(osvc, &service_id, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		printf("%s: Error waiting on %s (%s)\n", NAME, osvc,
		    str_error(rc));
		return rc;
	}

	return srv_start(SRV_CONSOLE, isvc, osvc);
}

#ifdef CONFIG_WINSYS

static errno_t display_server(void)
{
	return srv_start(SRV_DISPLAY);
}

static int app_start(const char *app, const char *arg)
{
	printf("%s: Spawning %s\n", NAME, app);

	task_id_t id;
	task_wait_t wait;
	errno_t rc = task_spawnl(&id, &wait, app, app, arg, NULL);
	if (rc != EOK) {
		oom_check(rc, app);
		printf("%s: Error spawning %s (%s)\n", NAME, app,
		    str_error(rc));
		return -1;
	}

	task_exit_t texit;
	int retval;
	rc = task_wait(&wait, &texit, &retval);
	if ((rc != EOK) || (texit != TASK_EXIT_NORMAL)) {
		printf("%s: Error retrieving retval from %s (%s)\n", NAME,
		    app, str_error(rc));
		return rc;
	}

	return retval;
}

#endif

static void getterm(const char *svc, const char *app, bool msg)
{
	if (msg) {
		printf("%s: Spawning %s %s %s --msg --wait -- %s\n", NAME,
		    APP_GETTERM, svc, LOCFS_MOUNT_POINT, app);

		errno_t rc = task_spawnl(NULL, NULL, APP_GETTERM, APP_GETTERM, svc,
		    LOCFS_MOUNT_POINT, "--msg", "--wait", "--", app, NULL);
		if (rc != EOK) {
			oom_check(rc, APP_GETTERM);
			printf("%s: Error spawning %s %s %s --msg --wait -- %s\n",
			    NAME, APP_GETTERM, svc, LOCFS_MOUNT_POINT, app);
		}
	} else {
		printf("%s: Spawning %s %s %s --wait -- %s\n", NAME,
		    APP_GETTERM, svc, LOCFS_MOUNT_POINT, app);

		errno_t rc = task_spawnl(NULL, NULL, APP_GETTERM, APP_GETTERM, svc,
		    LOCFS_MOUNT_POINT, "--wait", "--", app, NULL);
		if (rc != EOK) {
			oom_check(rc, APP_GETTERM);
			printf("%s: Error spawning %s %s %s --wait -- %s\n",
			    NAME, APP_GETTERM, svc, LOCFS_MOUNT_POINT, app);
		}
	}
}

static bool mount_tmpfs(void)
{
	errno_t rc = vfs_mount_path(TMPFS_MOUNT_POINT, TMPFS_FS_TYPE, "", "", 0, 0);
	return mount_report("Temporary file system", TMPFS_MOUNT_POINT,
	    TMPFS_FS_TYPE, NULL, rc);
}

/** Init system volume.
 *
 * See if system volume is configured. If so, try to wait for it to become
 * available. If not, create basic directories for live image omde.
 */
static errno_t init_sysvol(void)
{
	vol_t *vol = NULL;
	vol_info_t vinfo;
	volume_id_t *volume_ids = NULL;
	service_id_t *part_ids = NULL;
	vol_part_info_t pinfo;
	size_t nvols;
	size_t nparts;
	bool sv_mounted;
	futil_t *futil = NULL;
	size_t i;
	errno_t rc;
	bool found_cfg;
	const char **cp;

	rc = vol_create(&vol);
	if (rc != EOK) {
		printf("Error contacting volume service.\n");
		goto error;
	}

	rc = vol_get_volumes(vol, &volume_ids, &nvols);
	if (rc != EOK) {
		printf("Error getting list of volumes.\n");
		goto error;
	}

	/* XXX This could be handled more efficiently by volsrv itself */
	found_cfg = false;
	for (i = 0; i < nvols; i++) {
		rc = vol_info(vol, volume_ids[i], &vinfo);
		if (rc != EOK) {
			printf("Error getting volume information.\n");
			rc = EIO;
			goto error;
		}

		if (str_cmp(vinfo.path, "/w") == 0) {
			found_cfg = true;
			break;
		}
	}

	free(volume_ids);
	volume_ids = NULL;

	if (!found_cfg) {
		/* Prepare directory structure for live image mode */
		printf("%s: Creating live image directory structure.\n", NAME);
		cp = sys_dirs;
		while (*cp != NULL) {
			rc = vfs_link_path(*cp, KIND_DIRECTORY, NULL);
			if (rc != EOK) {
				printf("%s: Error creating directory '%s'.\n",
				    NAME, *cp);
				goto error;
			}

			++cp;
		}

		/* Copy initial configuration files */
		rc = futil_create(NULL, NULL, &futil);
		if (rc != EOK)
			goto error;

		rc = futil_rcopy_contents(futil, "/cfg", "/w/cfg");
		if (rc != EOK)
			goto error;

		futil_destroy(futil);
		futil = NULL;
	} else {
		printf("%s: System volume is configured.\n", NAME);

		/* Verify that system volume is mounted */
		sv_mounted = false;

		rc = vol_get_parts(vol, &part_ids, &nparts);
		if (rc != EOK) {
			printf("Error getting list of volumes.\n");
			goto error;
		}

		for (i = 0; i < nparts; i++) {
			rc = vol_part_info(vol, part_ids[i], &pinfo);
			if (rc != EOK) {
				printf("Error getting partition "
				    "information.\n");
				rc = EIO;
				goto error;
			}

			if (str_cmp(pinfo.cur_mp, "/w") == 0) {
				sv_mounted = true;
				break;
			}
		}

		if (sv_mounted == false) {
			printf("System volume not found.\n");
			rc = EIO;
			goto error;
		}

		free(part_ids);
		part_ids = NULL;

	}

	vol_destroy(vol);
	return EOK;
error:
	vol_destroy(vol);
	if (volume_ids != NULL)
		free(volume_ids);
	if (part_ids != NULL)
		free(part_ids);
	if (futil != NULL)
		futil_destroy(futil);

	return rc;
}

/** Perform sytem startup tasks.
 *
 * @return EOK on success or an error code
 */
static errno_t system_startup(void)
{
	errno_t rc;

	/* Make sure file systems are running. */
	if (str_cmp(STRING(RDFMT), "tmpfs") != 0)
		srv_start("/srv/fs/tmpfs");
	if (str_cmp(STRING(RDFMT), "exfat") != 0)
		srv_start("/srv/fs/exfat");
	if (str_cmp(STRING(RDFMT), "fat") != 0)
		srv_start("/srv/fs/fat");
	srv_start("/srv/fs/cdfs");
	srv_start("/srv/fs/mfs");

	srv_start("/srv/klog");
	srv_start("/srv/fs/locfs");

	if (!mount_locfs()) {
		printf("%s: Exiting\n", NAME);
		return EIO;
	}

	mount_tmpfs();

	srv_start("/srv/devman");
	srv_start("/srv/hid/s3c24xx_uart");
	srv_start("/srv/hid/s3c24xx_ts");

	srv_start("/srv/bd/vbd");
	srv_start("/srv/volsrv");

	init_sysvol();

	srv_start("/srv/taskmon");

	srv_start("/srv/net/loopip");
	srv_start("/srv/net/ethip");
	srv_start("/srv/net/dhcp");
	srv_start("/srv/net/inetsrv");
	srv_start("/srv/net/tcp");
	srv_start("/srv/net/udp");
	srv_start("/srv/net/dnsrsrv");

	srv_start("/srv/clipboard");
	srv_start("/srv/hid/remcons");

	srv_start("/srv/hid/input", HID_INPUT);
	srv_start("/srv/hid/output", HID_OUTPUT);
	srv_start("/srv/audio/hound");

#ifdef CONFIG_WINSYS
	if (!config_key_exists("console")) {
		rc = display_server();
		if (rc == EOK) {
			app_start("/app/taskbar", NULL);
			app_start("/app/terminal", "-topleft");
		}
	}
#endif
	rc = console(HID_INPUT, HID_OUTPUT);
	if (rc == EOK) {
		getterm("term/vc0", "/app/bdsh", true);
		getterm("term/vc1", "/app/bdsh", false);
		getterm("term/vc2", "/app/bdsh", false);
		getterm("term/vc3", "/app/bdsh", false);
		getterm("term/vc4", "/app/bdsh", false);
		getterm("term/vc5", "/app/bdsh", false);
	}

	return EOK;
}

/** Perform sytem shutdown tasks.
 *
 * @return EOK on success or an error code
 */
static errno_t system_sys_shutdown(void)
{
	vol_t *vol = NULL;
	service_id_t *part_ids = NULL;
	size_t nparts;
	size_t i;
	errno_t rc;

	/* Eject all volumes. */

	log_msg(LOG_DEFAULT, LVL_NOTE, "Ejecting volumes.");

	rc = vol_create(&vol);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error contacting volume "
		    "service.");
		goto error;
	}

	rc = vol_get_parts(vol, &part_ids, &nparts);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error getting volume list.");
		goto error;
	}

	for (i = 0; i < nparts; i++) {
		rc = vol_part_eject(vol, part_ids[i], vef_none);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_ERROR, "Error ejecting "
			    "volume %zu", (size_t)part_ids[i]);
			goto error;
		}
	}

	free(part_ids);
	vol_destroy(vol);

	return EOK;
error:
	if (part_ids != NULL)
		free(part_ids);
	if (vol != NULL)
		vol_destroy(vol);
	return rc;
}

/** Initialize system control service. */
static errno_t system_srv_init(sys_srv_t *syssrv)
{
	port_id_t port = 0;
	loc_srv_t *srv = NULL;
	service_id_t sid = 0;
	errno_t rc;

	(void)system;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "system_srv_init()");

	rc = async_create_port(INTERFACE_SYSTEM, system_srv_conn, syssrv,
	    &port);
	if (rc != EOK)
		goto error;

	rc = loc_server_register(NAME, &srv);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Failed registering server: %s.", str_error(rc));
		rc = EEXIST;
		goto error;
	}

	rc = loc_service_register(srv, SYSTEM_DEFAULT, port, &sid);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Failed registering service: %s.", str_error(rc));
		rc = EEXIST;
		goto error;
	}

	return EOK;
error:
	if (sid != 0)
		loc_service_unregister(srv, sid);
	if (srv != NULL)
		loc_server_unregister(srv);
	if (port != 0)
		async_port_destroy(port);
	return rc;
}

/** Handle connection to system server. */
static void system_srv_conn(ipc_call_t *icall, void *arg)
{
	sys_srv_t *syssrv = (sys_srv_t *)arg;

	/* Set up protocol structure */
	system_srv_initialize(&syssrv->srv);
	syssrv->srv.ops = &system_srv_ops;
	syssrv->srv.arg = syssrv;

	/* Handle connection */
	system_conn(icall, &syssrv->srv);
}

/** System poweroff request.
 *
 * @param arg Argument (sys_srv_t *)
 */
static errno_t system_srv_poweroff(void *arg)
{
	sys_srv_t *syssrv = (sys_srv_t *)arg;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "system_srv_poweroff");

	rc = system_sys_shutdown();
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_NOTE, "system_srv_poweroff failed");
		system_srv_shutdown_failed(&syssrv->srv);
	}

	log_msg(LOG_DEFAULT, LVL_NOTE, "system_srv_poweroff complete");
	system_srv_shutdown_complete(&syssrv->srv);
	return EOK;
}

/** System restart request.
 *
 * @param arg Argument (sys_srv_t *)
 */
static errno_t system_srv_restart(void *arg)
{
	sys_srv_t *syssrv = (sys_srv_t *)arg;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "system_srv_restart");

	rc = system_sys_shutdown();
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_NOTE, "system_srv_restart failed");
		system_srv_shutdown_failed(&syssrv->srv);
	}

	/* Quiesce the device tree. */

	log_msg(LOG_DEFAULT, LVL_NOTE, "Quiescing devices.");

	rc = devman_quiesce_devices("/hw");
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Failed to quiesce device tree.");
		return rc;
	}

	sys_reboot();

	log_msg(LOG_DEFAULT, LVL_NOTE, "system_srv_restart complete");
	system_srv_shutdown_complete(&syssrv->srv);
	return EOK;
}

int main(int argc, char *argv[])
{
	errno_t rc;
	sys_srv_t srv;

	info_print();

	if (log_init(NAME) != EOK) {
		printf(NAME ": Failed to initialize logging.\n");
		return 1;
	}

	/* Perform startup tasks. */
	rc = system_startup();
	if (rc != EOK)
		return 1;

	rc = system_srv_init(&srv);
	if (rc != EOK)
		return 1;

	printf(NAME ": Accepting connections.\n");
	task_retval(0);
	async_manager();

	return 0;
}

/** @}
 */
