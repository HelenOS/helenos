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
#include <driver.h>

#define NAME "test1"

static int add_device(device_t *dev);

static driver_ops_t driver_ops = {
	.add_device = &add_device
};

static driver_t the_driver = {
	.name = NAME,
	.driver_ops = &driver_ops
};

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
static int add_device(device_t *dev)
{
	int rc;
	printf(NAME ": add_device(name=\"%s\", handle=%d)\n",
	    dev->name, (int) dev->handle);

	if (dev->parent == NULL) {
		printf(NAME ": cloning myself ;-).\n");
		rc = child_device_register_wrapper(dev, "clone",
		    "virtual&test1", 10);
		if (rc != EOK) {
			printf(NAME ": failed to register clone (%s).\n",
			    str_error(rc));
		}
	} else if (str_cmp(dev->name, "clone") == 0) {
		printf(NAME ": registering child device run by the same task.\n");
		rc = child_device_register_wrapper(dev, "child",
		    "virtual&test1&child", 10);
		if (rc != EOK) {
			printf(NAME ": failed to register child (%s).\n",
			    str_error(rc));
		}
	}

	printf(NAME ": device `%s' accepted.\n", dev->name);

	return EOK;
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS test1 virtual device driver\n");
	return driver_main(&the_driver);
}


