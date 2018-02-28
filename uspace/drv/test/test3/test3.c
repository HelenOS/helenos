/*
 * Copyright (c) 2011 Vojtech Horky
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

#define NAME "test3"

#define NUM_FUNCS 20

static errno_t test3_dev_add(ddf_dev_t *dev);
static errno_t test3_dev_remove(ddf_dev_t *dev);
static errno_t test3_fun_online(ddf_fun_t *fun);
static errno_t test3_fun_offline(ddf_fun_t *fun);

static driver_ops_t driver_ops = {
	.dev_add = &test3_dev_add,
	.dev_remove = &test3_dev_remove,
	.fun_online = &test3_fun_online,
	.fun_offline = &test3_fun_offline,
};

static driver_t test3_driver = {
	.name = NAME,
	.driver_ops = &driver_ops
};

typedef struct {
	ddf_dev_t *dev;
	ddf_fun_t *fun[NUM_FUNCS];
} test3_t;

static errno_t register_fun_and_add_to_category(ddf_dev_t *parent,
    const char *base_name, size_t index, const char *class_name,
    ddf_fun_t **pfun)
{
	ddf_fun_t *fun = NULL;
	errno_t rc;
	char *fun_name = NULL;

	if (asprintf(&fun_name, "%s%zu", base_name, index) < 0) {
		ddf_msg(LVL_ERROR, "Failed to format string: No memory");
		rc = ENOMEM;
		goto leave;
	}

	fun = ddf_fun_create(parent, fun_exposed, fun_name);
	if (fun == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating function %s", fun_name);
		rc = ENOMEM;
		goto leave;
	}

	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function %s: %s", fun_name,
		    str_error(rc));
		goto leave;
	}

	ddf_fun_add_to_category(fun, class_name);

	ddf_msg(LVL_NOTE, "Registered exposed function `%s'.", fun_name);

leave:
	free(fun_name);

	if ((rc != EOK) && (fun != NULL)) {
		ddf_fun_destroy(fun);
	}

	*pfun = fun;
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

static errno_t test3_dev_add(ddf_dev_t *dev)
{
	errno_t rc = EOK;
	test3_t *test3;

	ddf_msg(LVL_DEBUG, "dev_add(name=\"%s\", handle=%d)",
	    ddf_dev_get_name(dev), (int) ddf_dev_get_handle(dev));

	test3 = ddf_dev_data_alloc(dev, sizeof(test3_t));
	if (test3 == NULL) {
		ddf_msg(LVL_ERROR, "Failed allocating soft state.");
		return ENOMEM;
	}

	size_t i;
	for (i = 0; i < NUM_FUNCS; i++) {
		rc = register_fun_and_add_to_category(dev,
		    "test3_", i, "test3", &test3->fun[i]);
		if (rc != EOK) {
			break;
		}
	}

	return rc;
}

static errno_t test3_dev_remove(ddf_dev_t *dev)
{
	test3_t *test3 = (test3_t *)ddf_dev_data_get(dev);
	char *fun_name;
	errno_t rc;
	size_t i;

	for (i = 0; i < NUM_FUNCS; i++) {
		if (asprintf(&fun_name, "test3_%zu", i) < 0) {
			ddf_msg(LVL_ERROR, "Failed to format string: No memory");
			return ENOMEM;
		}

		rc = fun_remove(test3->fun[i], fun_name);
		if (rc != EOK)
			return rc;

		free(fun_name);
	}

	return EOK;
}

static errno_t test3_fun_online(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "test3_fun_online()");
	return ddf_fun_online(fun);
}

static errno_t test3_fun_offline(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "test3_fun_offline()");
	return ddf_fun_offline(fun);
}


int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS test3 virtual device driver\n");
	ddf_log_init(NAME);
	return ddf_driver_main(&test3_driver);
}

