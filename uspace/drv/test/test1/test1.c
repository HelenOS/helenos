/*
 * Copyright (c) 2010 Vojtech Horky
 * Copyright (c) 2011 Jiri Svoboda
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
#include <ddf/log.h>
#include <str.h>

#include "test1.h"

static errno_t test1_dev_add(ddf_dev_t *dev);
static errno_t test1_dev_remove(ddf_dev_t *dev);
static errno_t test1_dev_gone(ddf_dev_t *dev);
static errno_t test1_fun_online(ddf_fun_t *fun);
static errno_t test1_fun_offline(ddf_fun_t *fun);

static driver_ops_t driver_ops = {
	.dev_add = &test1_dev_add,
	.dev_remove = &test1_dev_remove,
	.dev_gone = &test1_dev_gone,
	.fun_online = &test1_fun_online,
	.fun_offline = &test1_fun_offline
};

static driver_t test1_driver = {
	.name = NAME,
	.driver_ops = &driver_ops
};

typedef struct {
	ddf_fun_t *fun_a;
	ddf_fun_t *clone;
	ddf_fun_t *child;
} test1_t;

/** Register child and inform user about it.
 *
 * @param parent Parent device.
 * @param message Message for the user.
 * @param name Device name.
 * @param match_id Device match id.
 * @param score Device match score.
 */
static errno_t register_fun_verbose(ddf_dev_t *parent, const char *message,
    const char *name, const char *match_id, int match_score,
    errno_t expected_rc, ddf_fun_t **pfun)
{
	ddf_fun_t *fun = NULL;
	errno_t rc;

	ddf_msg(LVL_DEBUG, "Registering function `%s': %s.", name, message);

	fun = ddf_fun_create(parent, fun_inner, name);
	if (fun == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating function %s", name);
		rc = ENOMEM;
		goto leave;
	}

	rc = ddf_fun_add_match_id(fun, match_id, match_score);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed adding match IDs to function %s",
		    name);
		goto leave;
	}

	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function %s: %s", name,
		    str_error(rc));
		goto leave;
	}

	ddf_msg(LVL_NOTE, "Registered child device `%s'", name);
	rc = EOK;

leave:
	if (rc != expected_rc) {
		fprintf(stderr,
		    NAME ": Unexpected error registering function `%s'.\n"
		    NAME ":     Expected \"%s\" but got \"%s\".\n",
		    name, str_error(expected_rc), str_error(rc));
	}

	if ((rc != EOK) && (fun != NULL)) {
		ddf_fun_destroy(fun);
	}

	if (pfun != NULL)
		*pfun = fun;

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
 * and the DDF shall not deadlock.
 *
 *
 * @param dev New device.
 * @return Error code reporting success of the operation.
 */
static errno_t test1_dev_add(ddf_dev_t *dev)
{
	ddf_fun_t *fun_a;
	test1_t *test1;
	const char *dev_name;
	errno_t rc;

	dev_name = ddf_dev_get_name(dev);
	ddf_msg(LVL_DEBUG, "dev_add(name=\"%s\", handle=%d)",
	    dev_name, (int) ddf_dev_get_handle(dev));

	test1 = ddf_dev_data_alloc(dev, sizeof(test1_t));
	if (test1 == NULL) {
		ddf_msg(LVL_ERROR, "Failed allocating soft state.\n");
		rc = ENOMEM;
		goto error;
	}

	fun_a = ddf_fun_create(dev, fun_exposed, "a");
	if (fun_a == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating function 'a'.");
		rc = ENOMEM;
		goto error;
	}

	test1->fun_a = fun_a;

	rc = ddf_fun_bind(fun_a);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function 'a'.");
		ddf_fun_destroy(fun_a);
		goto error;
	}

	rc = ddf_fun_add_to_category(fun_a, "virtual");
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed adding function 'a' to category "
		    "'virtual'.");
		ddf_fun_unbind(fun_a);
		ddf_fun_destroy(fun_a);
		goto error;
	}

	if (str_cmp(dev_name, "test1") == 0) {
		(void) register_fun_verbose(dev,
		    "cloning myself ;-)", "clone",
		    "virtual&test1", 10, EOK, &test1->clone);
		(void) register_fun_verbose(dev,
		    "cloning myself twice ;-)", "clone",
		    "virtual&test1", 10, EEXIST, NULL);
	} else if (str_cmp(dev_name, "clone") == 0) {
		(void) register_fun_verbose(dev,
		    "run by the same task", "child",
		    "virtual&test1&child", 10, EOK, &test1->child);
	}

	ddf_msg(LVL_DEBUG, "Device `%s' accepted.", dev_name);
	return EOK;
error:
	return rc;
}

static errno_t fun_remove(ddf_fun_t *fun, const char *name)
{
	errno_t rc;

	ddf_msg(LVL_DEBUG, "fun_remove(%p, '%s')", fun, name);
	rc = ddf_fun_offline(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error offlining function '%s'.", name);
		return rc;
	}

	rc = ddf_fun_unbind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed unbinding function '%s'.", name);
		return rc;
	}

	ddf_fun_destroy(fun);
	return EOK;
}

static errno_t fun_unbind(ddf_fun_t *fun, const char *name)
{
	errno_t rc;

	ddf_msg(LVL_DEBUG, "fun_unbind(%p, '%s')", fun, name);
	rc = ddf_fun_unbind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed unbinding function '%s'.", name);
		return rc;
	}

	ddf_fun_destroy(fun);
	return EOK;
}

static errno_t test1_dev_remove(ddf_dev_t *dev)
{
	test1_t *test1 = (test1_t *)ddf_dev_data_get(dev);
	errno_t rc;

	ddf_msg(LVL_DEBUG, "test1_dev_remove(%p)", dev);

	if (test1->fun_a != NULL) {
		rc = fun_remove(test1->fun_a, "a");
		if (rc != EOK)
			return rc;
	}

	if (test1->clone != NULL) {
		rc = fun_remove(test1->clone, "clone");
		if (rc != EOK)
			return rc;
	}

	if (test1->child != NULL) {
		rc = fun_remove(test1->child, "child");
		if (rc != EOK)
			return rc;
	}

	return EOK;
}

static errno_t test1_dev_gone(ddf_dev_t *dev)
{
	test1_t *test1 = (test1_t *)ddf_dev_data_get(dev);
	errno_t rc;

	ddf_msg(LVL_DEBUG, "test1_dev_remove(%p)", dev);

	if (test1->fun_a != NULL) {
		rc = fun_unbind(test1->fun_a, "a");
		if (rc != EOK)
			return rc;
	}

	if (test1->clone != NULL) {
		rc = fun_unbind(test1->clone, "clone");
		if (rc != EOK)
			return rc;
	}

	if (test1->child != NULL) {
		rc = fun_unbind(test1->child, "child");
		if (rc != EOK)
			return rc;
	}

	return EOK;
}

static errno_t test1_fun_online(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "test1_fun_online()");
	return ddf_fun_online(fun);
}

static errno_t test1_fun_offline(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "test1_fun_offline()");
	return ddf_fun_offline(fun);
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS test1 virtual device driver\n");
	ddf_log_init(NAME);
	return ddf_driver_main(&test1_driver);
}
