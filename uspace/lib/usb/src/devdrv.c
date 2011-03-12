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

/** @addtogroup libusb
 * @{
 */
/** @file
 * USB device driver framework.
 */
#include <usb/devdrv.h>
#include <usb/request.h>
#include <usb/debug.h>
#include <errno.h>
#include <assert.h>

static int generic_add_device(ddf_dev_t *);

static driver_ops_t generic_driver_ops = {
	.add_device = generic_add_device
};
static driver_t generic_driver = {
	.driver_ops = &generic_driver_ops
};

static usb_driver_t *driver = NULL;

int usb_driver_main(usb_driver_t *drv)
{
	/* Prepare the generic driver. */
	generic_driver.name = drv->name;

	driver = drv;

	return ddf_driver_main(&generic_driver);
}

static size_t count_other_pipes(usb_driver_t *drv)
{
	size_t count = 0;
	if (drv->endpoints == NULL) {
		return 0;
	}

	while (drv->endpoints[count] != NULL) {
		count++;
	}

	return count;
}

static int initialize_other_pipes(usb_driver_t *drv, usb_device_t *dev)
{
	int my_interface = usb_device_get_assigned_interface(dev->ddf_dev);

	size_t pipe_count = count_other_pipes(drv);
	dev->pipes = malloc(sizeof(usb_endpoint_mapping_t) * pipe_count);
	assert(dev->pipes != NULL);

	size_t i;
	for (i = 0; i < pipe_count; i++) {
		dev->pipes[i].pipe = malloc(sizeof(usb_endpoint_pipe_t));
		assert(dev->pipes[i].pipe != NULL);
		dev->pipes[i].description = drv->endpoints[i];
		dev->pipes[i].interface_no = my_interface;
	}

	void *config_descriptor;
	size_t config_descriptor_size;
	int rc = usb_request_get_full_configuration_descriptor_alloc(
	   &dev->ctrl_pipe, 0, &config_descriptor, &config_descriptor_size);
	assert(rc == EOK);

	rc = usb_endpoint_pipe_initialize_from_configuration(dev->pipes,
	   pipe_count, config_descriptor, config_descriptor_size, &dev->wire);
	assert(rc == EOK);


	return EOK;
}

int generic_add_device(ddf_dev_t *gen_dev)
{
	assert(driver);
	assert(driver->ops);
	assert(driver->ops->add_device);

	int rc;

	usb_device_t *dev = malloc(sizeof(usb_device_t));
	assert(dev);

	dev->ddf_dev = gen_dev;
	dev->driver_data = NULL;

	/* Initialize the backing wire abstraction. */
	rc = usb_device_connection_initialize_from_device(&dev->wire, gen_dev);
	assert(rc == EOK);

	/* Initialize the default control pipe. */
	rc = usb_endpoint_pipe_initialize_default_control(&dev->ctrl_pipe,
	    &dev->wire);
	assert(rc == EOK);

	rc = usb_endpoint_pipe_start_session(&dev->ctrl_pipe);
	assert(rc == EOK);

	/* Initialize remaining pipes. */
	if (driver->endpoints != NULL) {
		initialize_other_pipes(driver, dev);
	}

	rc = usb_endpoint_pipe_end_session(&dev->ctrl_pipe);
	assert(rc == EOK);

	return driver->ops->add_device(dev);
}

/**
 * @}
 */
