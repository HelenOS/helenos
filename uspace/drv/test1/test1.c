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

/** @file
 */

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <str_error.h>
#include <ddf/driver.h>

#include "test1.h"

static int test1_add_device(ddf_dev_t *dev);

static driver_ops_t driver_ops = {
	.add_device = &test1_add_device
};

static driver_t test1_driver = {
	.name = NAME,
	.driver_ops = &driver_ops
};

/** Register child and inform user about it.
 *
 * @param parent Parent device.
 * @param message Message for the user.
 * @param name Device name.
 * @param match_id Device match id.
 * @param score Device match score.
 */
static int register_fun_verbose(ddf_dev_t *parent, const char *message,
    const char *name, const char *match_id, int match_score,
    int expected_rc)
{
	ddf_fun_t *fun = NULL;
	int rc;

	printf(NAME ": registering function `%s': %s.\n", name, message);

	fun = ddf_fun_create(parent, fun_inner, name);
	if (fun == NULL) {
		printf(NAME ": error creating function %s\n", name);
		rc = ENOMEM;
		goto leave;
	}

	rc = ddf_fun_add_match_id(fun, str_dup(match_id), match_score);
	if (rc != EOK) {
		printf(NAME ": error adding match IDs to function %s\n", name);
		goto leave;
	}

	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		printf(NAME ": error binding function %s: %s\n", name,
		    str_error(rc));
		goto leave;
	}

	printf(NAME ": registered child device `%s'\n", name);
	rc = EOK;


leave:
	if (rc != expected_rc) {
		fprintf(stderr,
		    NAME ": Unexpected error registering function `%s'.\n" \
		    NAME ":     Expected \"%s\" but got \"%s\".\n",
		    name, str_error(expected_rc), str_error(rc));
	}

	if ((rc != EOK) && (fun != NULL)) {
		ddf_fun_destroy(fun);
	}

	return rc;
}

/** Callback when new device is passed to this driver.
 * This function is the body of the test: it shall register new child
 * (named `clone') that shall be driven by the same task. When the clone
 * is added, it registers another child (named `child') that is also driven
 * by this task. The conditions ensure that we do not recurse indefinitely.
 * When successful, the device tree shall contain following fragment:
 *
 * /virtual/test1
 * /virtual/test1/clone
 * /virtual/test1/clone/child
 *
 * and devman shall not deadlock.
 *
 *
 * @param dev New device.
 * @return Error code reporting success of the operation.
 */
static int test1_add_device(ddf_dev_t *dev)
{
	ddf_fun_t *fun_a;
	int rc;

	printf(NAME ": add_device(name=\"%s\", handle=%d)\n",
	    dev->name, (int) dev->handle);

	fun_a = ddf_fun_create(dev, fun_exposed, "a");
	if (fun_a == NULL) {
		printf(NAME ": error creating function 'a'.\n");
		return ENOMEM;
	}

	rc = ddf_fun_bind(fun_a);
	if (rc != EOK) {
		printf(NAME ": error binding function 'a'.\n");
		return rc;
	}

	ddf_fun_add_to_class(fun_a, "virtual");

	if (str_cmp(dev->name, "null") == 0) {
		fun_a->ops = &char_device_ops;
		ddf_fun_add_to_class(fun_a, "virt-null");
	} else if (str_cmp(dev->name, "test1") == 0) {
		(void) register_fun_verbose(dev,
		    "cloning myself ;-)", "clone",
		    "virtual&test1", 10, EOK);
		(void) register_fun_verbose(dev,
		    "cloning myself twice ;-)", "clone",
		    "virtual&test1", 10, EEXISTS);
	} else if (str_cmp(dev->name, "clone") == 0) {
		(void) register_fun_verbose(dev,
		    "run by the same task", "child",
		    "virtual&test1&child", 10, EOK);
	}

	printf(NAME ": device `%s' accepted.\n", dev->name);

	return EOK;
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS test1 virtual device driver\n");
	return ddf_driver_main(&test1_driver);
}

