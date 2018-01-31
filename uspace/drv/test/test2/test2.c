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
#include <async.h>
#include <stdio.h>
#include <errno.h>
#include <str.h>
#include <str_error.h>
#include <ddf/driver.h>
#include <ddf/log.h>

#define NAME "test2"

static errno_t test2_dev_add(ddf_dev_t *dev);
static errno_t test2_dev_remove(ddf_dev_t *dev);
static errno_t test2_dev_gone(ddf_dev_t *dev);
static errno_t test2_fun_online(ddf_fun_t *fun);
static errno_t test2_fun_offline(ddf_fun_t *fun);

static driver_ops_t driver_ops = {
	.dev_add = &test2_dev_add,
	.dev_remove = &test2_dev_remove,
	.dev_gone = &test2_dev_gone,
	.fun_online = &test2_fun_online,
	.fun_offline = &test2_fun_offline
};

static driver_t test2_driver = {
	.name = NAME,
	.driver_ops = &driver_ops
};

/* Device soft state */
typedef struct {
	ddf_dev_t *dev;

	ddf_fun_t *fun_a;
	ddf_fun_t *fun_err;
	ddf_fun_t *child;
	ddf_fun_t *test1;
} test2_t;

/** Register child and inform user about it.
 *
 * @param parent Parent device.
 * @param message Message for the user.
 * @param name Device name.
 * @param match_id Device match id.
 * @param score Device match score.
 */
static errno_t register_fun_verbose(ddf_dev_t *parent, const char *message,
    const char *name, const char *match_id, int match_score, ddf_fun_t **pfun)
{
	ddf_fun_t *fun;
	errno_t rc;

	ddf_msg(LVL_DEBUG, "Registering function `%s': %s.", name, message);

	fun = ddf_fun_create(parent, fun_inner, name);
	if (fun == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating function %s", name);
		return ENOMEM;
	}

	rc = ddf_fun_add_match_id(fun, match_id, match_score);
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

	*pfun = fun;

	ddf_msg(LVL_NOTE, "Registered child device `%s'", name);
	return EOK;
}

/** Simulate plugging and surprise unplugging.
 *
 * @param arg Parent device structure (ddf_dev_t *).
 * @return Always EOK.
 */
static errno_t plug_unplug(void *arg)
{
	test2_t *test2 = (test2_t *) arg;
	ddf_fun_t *fun_a;
	errno_t rc;

	async_usleep(1000);

	(void) register_fun_verbose(test2->dev, "child driven by the same task",
	    "child", "virtual&test2", 10, &test2->child);
	(void) register_fun_verbose(test2->dev, "child driven by test1",
	    "test1", "virtual&test1", 10, &test2->test1);

	fun_a = ddf_fun_create(test2->dev, fun_exposed, "a");
	if (fun_a == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating function 'a'.");
		return ENOMEM;
	}

	rc = ddf_fun_bind(fun_a);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function 'a'.");
		return rc;
	}

	ddf_fun_add_to_category(fun_a, "virtual");
	test2->fun_a = fun_a;

	async_usleep(10000000);

	ddf_msg(LVL_NOTE, "Unbinding function test1.");
	ddf_fun_unbind(test2->test1);
	async_usleep(1000000);
	ddf_msg(LVL_NOTE, "Unbinding function child.");
	ddf_fun_unbind(test2->child);

	return EOK;
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

static errno_t test2_dev_add(ddf_dev_t *dev)
{
	test2_t *test2;
	const char *dev_name = ddf_dev_get_name(dev);

	ddf_msg(LVL_DEBUG, "test2_dev_add(name=\"%s\", handle=%d)",
	    dev_name, (int) ddf_dev_get_handle(dev));

	test2 = ddf_dev_data_alloc(dev, sizeof(test2_t));
	if (test2 == NULL) {
		ddf_msg(LVL_ERROR, "Failed allocating soft state.");
		return ENOMEM;
	}

	test2->dev = dev;

	if (str_cmp(dev_name, "child") != 0) {
		fid_t postpone = fibril_create(plug_unplug, test2);
		if (postpone == 0) {
			ddf_msg(LVL_ERROR, "fibril_create() failed.");
			return ENOMEM;
		}
		fibril_add_ready(postpone);
	} else {
		(void) register_fun_verbose(dev, "child without available driver",
		    "ERROR", "non-existent.match.id", 10, &test2->fun_err);
	}

	return EOK;
}

static errno_t test2_dev_remove(ddf_dev_t *dev)
{
	test2_t *test2 = (test2_t *)ddf_dev_data_get(dev);
	errno_t rc;

	ddf_msg(LVL_DEBUG, "test2_dev_remove(%p)", dev);

	if (test2->fun_a != NULL) {
		rc = fun_remove(test2->fun_a, "a");
		if (rc != EOK)
			return rc;
	}

	if (test2->fun_err != NULL) {
		rc = fun_remove(test2->fun_err, "ERROR");
		if (rc != EOK)
			return rc;
	}

	if (test2->child != NULL) {
		rc = fun_remove(test2->child, "child");
		if (rc != EOK)
			return rc;
	}

	if (test2->test1 != NULL) {
		rc = fun_remove(test2->test1, "test1");
		if (rc != EOK)
			return rc;
	}

	return EOK;
}

static errno_t test2_dev_gone(ddf_dev_t *dev)
{
	test2_t *test2 = (test2_t *)ddf_dev_data_get(dev);
	errno_t rc;

	ddf_msg(LVL_DEBUG, "test2_dev_gone(%p)", dev);

	if (test2->fun_a != NULL) {
		rc = fun_unbind(test2->fun_a, "a");
		if (rc != EOK)
			return rc;
	}

	if (test2->fun_err != NULL) {
		rc = fun_unbind(test2->fun_err, "ERROR");
		if (rc != EOK)
			return rc;
	}

	if (test2->child != NULL) {
		rc = fun_unbind(test2->child, "child");
		if (rc != EOK)
			return rc;
	}

	if (test2->test1 != NULL) {
		rc = fun_unbind(test2->test1, "test1");
		if (rc != EOK)
			return rc;
	}

	return EOK;
}


static errno_t test2_fun_online(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "test2_fun_online()");
	return ddf_fun_online(fun);
}

static errno_t test2_fun_offline(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "test2_fun_offline()");
	return ddf_fun_offline(fun);
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS test2 virtual device driver\n");
	ddf_log_init(NAME);
	return ddf_driver_main(&test2_driver);
}


