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

static int test3_add_device(ddf_dev_t *dev);

static driver_ops_t driver_ops = {
	.add_device = &test3_add_device
};

static driver_t test3_driver = {
	.name = NAME,
	.driver_ops = &driver_ops
};

static int register_fun_and_add_to_category(ddf_dev_t *parent,
     const char *base_name, size_t index, const char *class_name)
{
	ddf_fun_t *fun = NULL;
	int rc;
	char *fun_name = NULL;
	
	rc = asprintf(&fun_name, "%s%zu", base_name, index);
	if (rc < 0) {
		ddf_msg(LVL_ERROR, "Failed to format string: %s", str_error(rc));
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
	
	return rc;
}

static int test3_add_device(ddf_dev_t *dev)
{
	int rc = EOK;

	ddf_msg(LVL_DEBUG, "add_device(name=\"%s\", handle=%d)",
	    dev->name, (int) dev->handle);

	size_t i;
	for (i = 0; i < 20; i++) {
		rc = register_fun_and_add_to_category(dev,
		    "test3_", i, "test3");
		if (rc != EOK) {
			break;
		}
	}
	
	return rc;
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS test3 virtual device driver\n");
	ddf_log_init(NAME, LVL_ERROR);
	return ddf_driver_main(&test3_driver);
}

