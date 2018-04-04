/*
 * Copyright (c) 2010 Vojtech Horky
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

/**
 * @defgroup virt Root device driver for virtual devices.
 * @{
 */

/** @file
 */

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <str_error.h>
#include <ddf/driver.h>
#include <ddf/log.h>

#define NAME "virt"

/** Virtual function entry */
typedef struct {
	/** Function name */
	const char *name;
	/** Function match ID */
	const char *match_id;
} virtual_function_t;

/** List of existing virtual functions */
virtual_function_t virtual_functions[] = {
#include "devices.def"
	/* Terminating item */
	{
		.name = NULL,
		.match_id = NULL
	}
};

static errno_t virt_dev_add(ddf_dev_t *dev);
static errno_t virt_dev_remove(ddf_dev_t *dev);
static errno_t virt_fun_online(ddf_fun_t *fun);
static errno_t virt_fun_offline(ddf_fun_t *fun);

static driver_ops_t virt_ops = {
	.dev_add = &virt_dev_add,
	.dev_remove = &virt_dev_remove,
	.fun_online = &virt_fun_online,
	.fun_offline = &virt_fun_offline
};

static driver_t virt_driver = {
	.name = NAME,
	.driver_ops = &virt_ops
};

/* Device soft state */
typedef struct {
	ddf_dev_t *dev;
	list_t functions;
} virt_t;

/* Function soft state */
typedef struct {
	ddf_fun_t *fun;
	link_t dev_link;
} virt_fun_t;

static int instances = 0;


/** Add function to the virtual device.
 *
 * @param vdev		The virtual device
 * @param vfun		Virtual function description
 * @return		EOK on success or an error code.
 */
static errno_t virt_add_fun(virt_t *virt, virtual_function_t *vfun)
{
	ddf_dev_t *vdev = virt->dev;
	ddf_fun_t *fun;
	virt_fun_t *rvfun;
	errno_t rc;

	ddf_msg(LVL_DEBUG, "Registering function `%s' (match \"%s\")",
	    vfun->name, vfun->match_id);

	fun = ddf_fun_create(vdev, fun_inner, vfun->name);
	if (fun == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating function %s", vfun->name);
		return ENOMEM;
	}

	rvfun = ddf_fun_data_alloc(fun, sizeof(virt_fun_t));
	if (rvfun == NULL) {
		ddf_msg(LVL_ERROR, "Failed allocating soft state for %s.",
		    vfun->name);
		ddf_fun_destroy(fun);
		return ENOMEM;
	}

	rvfun->fun = fun;

	rc = ddf_fun_add_match_id(fun, vfun->match_id, 10);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed adding match IDs to function %s",
		    vfun->name);
		ddf_fun_destroy(fun);
		return rc;
	}

	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function %s: %s",
		    vfun->name, str_error(rc));
		ddf_fun_destroy(fun);
		return rc;
	}

	list_append(&rvfun->dev_link, &virt->functions);

	ddf_msg(LVL_NOTE, "Registered child device `%s'", vfun->name);
	return EOK;
}

static errno_t virt_fun_remove(virt_fun_t *rvfun)
{
	errno_t rc;
	const char *name = ddf_fun_get_name(rvfun->fun);

	ddf_msg(LVL_DEBUG, "virt_fun_remove('%s')", name);
	rc = ddf_fun_offline(rvfun->fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error offlining function '%s'.", name);
		return rc;
	}

	rc = ddf_fun_unbind(rvfun->fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed unbinding function '%s'.", name);
		return rc;
	}

	list_remove(&rvfun->dev_link);
	ddf_fun_destroy(rvfun->fun);
	return EOK;
}


static errno_t virt_dev_add(ddf_dev_t *dev)
{
	virt_t *virt;

	/*
	 * Allow only single instance of root virtual device.
	 */
	if (++instances > 1) {
		return ELIMIT;
	}

	ddf_msg(LVL_DEBUG, "dev_add(handle=%d)", (int)ddf_dev_get_handle(dev));

	virt = ddf_dev_data_alloc(dev, sizeof(virt_t));
	if (virt == NULL)
		return ENOMEM;

	virt->dev = dev;
	list_initialize(&virt->functions);

	/*
	 * Go through all virtual functions and try to add them.
	 * We silently ignore failures.
	 */
	virtual_function_t *vfun = virtual_functions;
	while (vfun->name != NULL) {
		(void) virt_add_fun(virt, vfun);
		vfun++;
	}

	return EOK;
}

static errno_t virt_dev_remove(ddf_dev_t *dev)
{
	virt_t *virt = (virt_t *)ddf_dev_data_get(dev);
	errno_t rc;

	while (!list_empty(&virt->functions)) {
		virt_fun_t *rvfun = list_get_instance(
		    list_first(&virt->functions), virt_fun_t,
		    dev_link);

		rc = virt_fun_remove(rvfun);
		if (rc != EOK)
			return rc;
	}

	--instances;
	return EOK;
}

static errno_t virt_fun_online(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "virt_fun_online()");
	return ddf_fun_online(fun);
}

static errno_t virt_fun_offline(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "virt_fun_offline()");
	return ddf_fun_offline(fun);
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS virtual devices root driver\n");

	ddf_log_init(NAME);
	return ddf_driver_main(&virt_driver);
}

/**
 * @}
 */

