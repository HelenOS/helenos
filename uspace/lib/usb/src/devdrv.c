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
#include <str_error.h>
#include <assert.h>

static int generic_add_device(ddf_dev_t *);

static driver_ops_t generic_driver_ops = {
	.add_device = generic_add_device
};
static driver_t generic_driver = {
	.driver_ops = &generic_driver_ops
};

static usb_driver_t *driver = NULL;


/** Main routine of USB device driver.
 *
 * Under normal conditions, this function never returns.
 *
 * @param drv USB device driver structure.
 * @return Task exit status.
 */
int usb_driver_main(usb_driver_t *drv)
{
	assert(drv != NULL);

	/* Prepare the generic driver. */
	generic_driver.name = drv->name;

	driver = drv;

	return ddf_driver_main(&generic_driver);
}

/** Log out of memory error on given device.
 *
 * @param dev Device causing the trouble.
 */
static void usb_log_oom(ddf_dev_t *dev)
{
	usb_log_error("Out of memory when adding device `%s'.\n",
	    dev->name);
}

/** Count number of pipes the driver expects.
 *
 * @param drv USB driver.
 * @return Number of pipes (excluding default control pipe).
 */
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

/** Initialize endpoint pipes, excluding default control one.
 *
 * @param drv The device driver.
 * @param dev Device to be initialized.
 * @return Error code.
 */
static int initialize_other_pipes(usb_driver_t *drv, usb_device_t *dev)
{
	int rc;
	int my_interface = usb_device_get_assigned_interface(dev->ddf_dev);

	size_t pipe_count = count_other_pipes(drv);
	dev->pipes = malloc(sizeof(usb_endpoint_mapping_t) * pipe_count);
	if (dev->pipes == NULL) {
		usb_log_oom(dev->ddf_dev);
		return ENOMEM;
	}

	size_t i;

	/* Initialize to NULL first for rollback purposes. */
	for (i = 0; i < pipe_count; i++) {
		dev->pipes[i].pipe = NULL;
	}

	for (i = 0; i < pipe_count; i++) {
		dev->pipes[i].pipe = malloc(sizeof(usb_endpoint_pipe_t));
		if (dev->pipes[i].pipe == NULL) {
			usb_log_oom(dev->ddf_dev);
			rc = ENOMEM;
			goto rollback;
		}

		dev->pipes[i].description = drv->endpoints[i];
		dev->pipes[i].interface_no = my_interface;
	}

	void *config_descriptor;
	size_t config_descriptor_size;
	rc = usb_request_get_full_configuration_descriptor_alloc(
	    &dev->ctrl_pipe, 0, &config_descriptor, &config_descriptor_size);
	if (rc != EOK) {
		usb_log_error("Failed retrieving configuration of `%s': %s.\n",
		    dev->ddf_dev->name, str_error(rc));
		goto rollback;
	}

	rc = usb_endpoint_pipe_initialize_from_configuration(dev->pipes,
	   pipe_count, config_descriptor, config_descriptor_size, &dev->wire);
	if (rc != EOK) {
		usb_log_error("Failed initializing USB endpoints: %s.\n",
		    str_error(rc));
		goto rollback;
	}

	return EOK;

rollback:
	for (i = 0; i < pipe_count; i++) {
		if (dev->pipes[i].pipe != NULL) {
			free(dev->pipes[i].pipe);
		}
	}
	free(dev->pipes);

	return rc;
}

/** Initialize all endpoint pipes.
 *
 * @param drv The driver.
 * @param dev The device to be initialized.
 * @return Error code.
 */
static int initialize_pipes(usb_driver_t *drv, usb_device_t *dev)
{
	int rc;

	rc = usb_device_connection_initialize_from_device(&dev->wire,
	    dev->ddf_dev);
	if (rc != EOK) {
		usb_log_error(
		    "Failed initializing connection on device `%s'. %s.\n",
		    dev->ddf_dev->name, str_error(rc));
		return rc;
	}

	rc = usb_endpoint_pipe_initialize_default_control(&dev->ctrl_pipe,
	    &dev->wire);
	if (rc != EOK) {
		usb_log_error("Failed to initialize default control pipe " \
		    "on device `%s': %s.\n",
		    dev->ddf_dev->name, str_error(rc));
		return rc;
	}

	/*
	 * Initialization of other pipes requires open session on
	 * default control pipe.
	 */
	rc = usb_endpoint_pipe_start_session(&dev->ctrl_pipe);
	if (rc != EOK) {
		usb_log_error("Failed to start an IPC session: %s.\n",
		    str_error(rc));
		return rc;
	}

	if (driver->endpoints != NULL) {
		rc = initialize_other_pipes(driver, dev);
	}

	/* No checking here. */
	usb_endpoint_pipe_end_session(&dev->ctrl_pipe);

	return rc;
}

/** Callback when new device is supposed to be controlled by this driver.
 *
 * This callback is a wrapper for USB specific version of @c add_device.
 *
 * @param gen_dev Device structure as prepared by DDF.
 * @return Error code.
 */
int generic_add_device(ddf_dev_t *gen_dev)
{
	assert(driver);
	assert(driver->ops);
	assert(driver->ops->add_device);

	int rc;

	usb_device_t *dev = malloc(sizeof(usb_device_t));
	if (dev == NULL) {
		usb_log_error("Out of memory when adding device `%s'.\n",
		    gen_dev->name);
		return ENOMEM;
	}


	dev->ddf_dev = gen_dev;
	dev->ddf_dev->driver_data = dev;
	dev->driver_data = NULL;

	rc = initialize_pipes(driver, dev);
	if (rc != EOK) {
		free(dev);
		return rc;
	}

	return driver->ops->add_device(dev);
}

/**
 * @}
 */
