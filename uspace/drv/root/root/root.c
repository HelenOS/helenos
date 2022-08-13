/*
 * SPDX-FileCopyrightText: 2010 Lenka Trochtova
 * SPDX-FileCopyrightText: 2010 Vojtech Horky
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @addtogroup root
 * @{
 */

/** @file
 */

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <fibril_synch.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>
#include <ctype.h>
#include <macros.h>
#include <inttypes.h>
#include <sysinfo.h>

#include <ddf/driver.h>
#include <ddf/log.h>

#define NAME "root"

#define PLATFORM_FUN_NAME "hw"
#define PLATFORM_FUN_MATCH_ID_FMT "platform/%s"
#define PLATFORM_FUN_MATCH_SCORE 100

#define VIRTUAL_FUN_NAME "virt"
#define VIRTUAL_FUN_MATCH_ID "virt"
#define VIRTUAL_FUN_MATCH_SCORE 100

static errno_t root_dev_add(ddf_dev_t *dev);
static errno_t root_fun_online(ddf_fun_t *fun);
static errno_t root_fun_offline(ddf_fun_t *fun);

/** The root device driver's standard operations. */
static driver_ops_t root_ops = {
	.dev_add = &root_dev_add,
	.fun_online = &root_fun_online,
	.fun_offline = &root_fun_offline
};

/** The root device driver structure. */
static driver_t root_driver = {
	.name = NAME,
	.driver_ops = &root_ops
};

/** Create the function which represents the root of virtual device tree.
 *
 * @param dev	Device
 * @return	EOK on success or an error code
 */
static errno_t add_virtual_root_fun(ddf_dev_t *dev)
{
	const char *name = VIRTUAL_FUN_NAME;
	ddf_fun_t *fun;
	errno_t rc;

	ddf_msg(LVL_DEBUG, "Adding new function for virtual devices. "
	    "Function node is `%s' (%d %s)", name,
	    VIRTUAL_FUN_MATCH_SCORE, VIRTUAL_FUN_MATCH_ID);

	fun = ddf_fun_create(dev, fun_inner, name);
	if (fun == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating function %s", name);
		return ENOMEM;
	}

	rc = ddf_fun_add_match_id(fun, VIRTUAL_FUN_MATCH_ID,
	    VIRTUAL_FUN_MATCH_SCORE);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed adding match IDs to function %s",
		    name);
		ddf_fun_destroy(fun);
		return rc;
	}

	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function %s: %s", name,
		    str_error(rc));
		ddf_fun_destroy(fun);
		return rc;
	}

	return EOK;
}

/** Create the function which represents the root of HW device tree.
 *
 * @param dev	Device
 * @return	EOK on success or an error code
 */
static errno_t add_platform_fun(ddf_dev_t *dev)
{
	char *match_id;
	char *platform;
	size_t platform_size;

	const char *name = PLATFORM_FUN_NAME;
	ddf_fun_t *fun;
	errno_t rc;

	/* Get platform name from sysinfo. */
	platform = sysinfo_get_data("platform", &platform_size);
	if (platform == NULL) {
		ddf_msg(LVL_ERROR, "Failed to obtain platform name.");
		return ENOENT;
	}

	/* Null-terminate string. */
	char *tmp = realloc(platform, platform_size + 1);
	if (tmp == NULL) {
		ddf_msg(LVL_ERROR, "Memory allocation failed.");
		free(platform);
		return ENOMEM;
	}
	platform = tmp;
	platform[platform_size] = '\0';

	/* Construct match ID. */
	if (asprintf(&match_id, PLATFORM_FUN_MATCH_ID_FMT, platform) == -1) {
		ddf_msg(LVL_ERROR, "Memory allocation failed.");
		free(platform);
		return ENOMEM;
	}

	free(platform);

	/* Add function. */
	ddf_msg(LVL_DEBUG, "Adding platform function. Function node is `%s' "
	    " (%d %s)", PLATFORM_FUN_NAME, PLATFORM_FUN_MATCH_SCORE,
	    match_id);

	fun = ddf_fun_create(dev, fun_inner, name);
	if (fun == NULL) {
		ddf_msg(LVL_ERROR, "Error creating function %s", name);
		free(match_id);
		return ENOMEM;
	}

	rc = ddf_fun_add_match_id(fun, match_id, PLATFORM_FUN_MATCH_SCORE);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed adding match IDs to function %s",
		    name);
		free(match_id);
		ddf_fun_destroy(fun);
		return rc;
	}

	free(match_id);

	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function %s: %s", name,
		    str_error(rc));
		ddf_fun_destroy(fun);
		return rc;
	}

	return EOK;
}

/** Get the root device.
 *
 * @param dev		The device which is root of the whole device tree (both
 *			of HW and pseudo devices).
 */
static errno_t root_dev_add(ddf_dev_t *dev)
{
	ddf_msg(LVL_DEBUG, "root_dev_add, device handle=%" PRIun,
	    ddf_dev_get_handle(dev));

	/*
	 * Register virtual devices root.
	 * We warn on error occurrence because virtual devices shall not be
	 * vital for the system.
	 */
	errno_t res = add_virtual_root_fun(dev);
	if (res != EOK)
		ddf_msg(LVL_WARN, "Failed to add virtual child.");

	/* Register root device's children. */
	res = add_platform_fun(dev);
	if (EOK != res)
		ddf_msg(LVL_ERROR, "Failed adding child device for platform.");

	return res;
}

static errno_t root_fun_online(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "root_fun_online()");
	return ddf_fun_online(fun);
}

static errno_t root_fun_offline(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "root_fun_offline()");
	return ddf_fun_offline(fun);
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS root device driver\n");

	ddf_log_init(NAME);
	return ddf_driver_main(&root_driver);
}

/**
 * @}
 */
