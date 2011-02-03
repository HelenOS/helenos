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

#define NAME "test2"

static int add_device(device_t *dev);

static driver_ops_t driver_ops = {
	.add_device = &add_device
};

static driver_t the_driver = {
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
static void register_child_verbose(device_t *parent, const char *message,
    const char *name, const char *match_id, int match_score)
{
	printf(NAME ": registering child device `%s': %s.\n",
	   name, message);

	int rc = child_device_register_wrapper(parent, name,
	    match_id, match_score);

	if (rc == EOK) {
		printf(NAME ": registered child device `%s'.\n", name);
	} else {
		printf(NAME ": failed to register child `%s' (%s).\n",
		    name, str_error(rc));
	}
}

/** Add child devices after some sleep.
 *
 * @param arg Parent device structure (device_t *).
 * @return Always EOK.
 */
static int postponed_birth(void *arg)
{
	device_t *dev = (device_t *) arg;

	async_usleep(1000);

	register_child_verbose(dev, "child driven by the same task",
	    "child", "virtual&test2", 10);
	register_child_verbose(dev, "child driven by test1",
	    "test1", "virtual&test1", 10);

	add_device_to_class(dev, "virtual");

	return EOK;
}


static int add_device(device_t *dev)
{
	printf(NAME ": add_device(name=\"%s\", handle=%d)\n",
	    dev->name, (int) dev->handle);

	if (dev->parent == NULL) {
		fid_t postpone = fibril_create(postponed_birth, dev);
		if (postpone == 0) {
			printf(NAME ": fibril_create() error\n");
			return ENOMEM;
		}
		fibril_add_ready(postpone);
	} else {
		register_child_verbose(dev, "child without available driver",
		    "ERROR", "non-existent.match.id", 10);
	}

	return EOK;
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS test2 virtual device driver\n");
	return driver_main(&the_driver);
}


