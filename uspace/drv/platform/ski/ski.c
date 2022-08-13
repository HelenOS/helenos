/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @addtogroup ski ski
 * @{
 */

/** @file
 */

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include <ddf/driver.h>
#include <ddf/log.h>

#define NAME "ski"

static errno_t ski_dev_add(ddf_dev_t *dev);

static driver_ops_t ski_ops = {
	.dev_add = &ski_dev_add
};

static driver_t ski_driver = {
	.name = NAME,
	.driver_ops = &ski_ops
};

static errno_t ski_add_fun(ddf_dev_t *dev, const char *name, const char *str_match_id)
{
	ddf_msg(LVL_NOTE, "Adding function '%s'.", name);

	ddf_fun_t *fnode = NULL;
	errno_t rc;

	/* Create new device. */
	fnode = ddf_fun_create(dev, fun_inner, name);
	if (fnode == NULL) {
		ddf_msg(LVL_ERROR, "Error creating function '%s'", name);
		rc = ENOMEM;
		goto error;
	}

	/* Add match ID */
	rc = ddf_fun_add_match_id(fnode, str_match_id, 100);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error adding match ID");
		goto error;
	}

	/* Register function. */
	if (ddf_fun_bind(fnode) != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function %s.", name);
		goto error;
	}

	return EOK;

error:
	if (fnode != NULL)
		ddf_fun_destroy(fnode);

	return rc;
}

static errno_t ski_add_functions(ddf_dev_t *dev)
{
	errno_t rc;

	rc = ski_add_fun(dev, "console", "ski/console");
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Add device. */
static errno_t ski_dev_add(ddf_dev_t *dev)
{
	ddf_msg(LVL_NOTE, "ski_dev_add, device handle = %d",
	    (int)ddf_dev_get_handle(dev));

	/* Register functions. */
	if (ski_add_functions(dev)) {
		ddf_msg(LVL_ERROR, "Failed to add functions for ski platform.");
	}

	return EOK;
}

int main(int argc, char *argv[])
{
	errno_t rc;

	printf(NAME ": Ski platform driver\n");

	rc = ddf_log_init(NAME);
	if (rc != EOK) {
		printf(NAME ": Failed initializing logging service");
		return 1;
	}

	return ddf_driver_main(&ski_driver);
}

/**
 * @}
 */
