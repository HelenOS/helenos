/*
 * Copyright (c) 2011 Vojtech Horky
 * Copyright (c) 2011 Jan Vesely
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
/** @addtogroup libusbdev
 * @{
 */
/** @file
 * USB device driver framework.
 */
#include <usb/dev/driver.h>
#include <usb/dev/request.h>
#include <usb/debug.h>
#include <usb/dev.h>
#include <errno.h>
#include <str_error.h>
#include <assert.h>

static int generic_device_add(ddf_dev_t *);
static int generic_device_remove(ddf_dev_t *);
static int generic_device_gone(ddf_dev_t *);

static driver_ops_t generic_driver_ops = {
	.dev_add = generic_device_add,
	.dev_remove = generic_device_remove,
	.dev_gone = generic_device_gone,
};
static driver_t generic_driver = {
	.driver_ops = &generic_driver_ops
};

static const usb_driver_t *driver = NULL;

/** Main routine of USB device driver.
 *
 * Under normal conditions, this function never returns.
 *
 * @param drv USB device driver structure.
 * @return Task exit status.
 */
int usb_driver_main(const usb_driver_t *drv)
{
	assert(drv != NULL);

	/* Prepare the generic driver. */
	generic_driver.name = drv->name;

	driver = drv;

	return ddf_driver_main(&generic_driver);
}

/** Count number of pipes the driver expects.
 *
 * @param drv USB driver.
 * @return Number of pipes (excluding default control pipe).
 */
static inline size_t count_other_pipes(
    const usb_endpoint_description_t **endpoints)
{
	size_t count;
	for (count = 0; endpoints != NULL && endpoints[count] != NULL; ++count);
	return count;
}

/** Callback when a new device is supposed to be controlled by this driver.
 *
 * This callback is a wrapper for USB specific version of @c device_add.
 *
 * @param gen_dev Device structure as prepared by DDF.
 * @return Error code.
 */
int generic_device_add(ddf_dev_t *gen_dev)
{
	assert(driver);
	assert(driver->ops);
	assert(driver->ops->device_add);

	/* Get place for driver data. */
	usb_device_t *dev = ddf_dev_data_alloc(gen_dev, sizeof(usb_device_t));
	if (dev == NULL) {
		usb_log_error("USB device `%s' structure allocation failed.\n",
		    ddf_dev_get_name(gen_dev));
		return ENOMEM;
	}

	/* Initialize generic USB driver data. */
	const char *err_msg = NULL;
	int rc = usb_device_init(dev, gen_dev, driver->endpoints, &err_msg);
	if (rc != EOK) {
		usb_log_error("USB device `%s' init failed (%s): %s.\n",
		    ddf_dev_get_name(gen_dev), err_msg, str_error(rc));
		return rc;
	}

	/* Start USB driver specific initialization. */
	rc = driver->ops->device_add(dev);
	if (rc != EOK)
		usb_device_deinit(dev);
	return rc;
}

/** Callback when a device is supposed to be removed from the system.
 *
 * This callback is a wrapper for USB specific version of @c device_remove.
 *
 * @param gen_dev Device structure as prepared by DDF.
 * @return Error code.
 */
int generic_device_remove(ddf_dev_t *gen_dev)
{
	assert(driver);
	assert(driver->ops);
	if (driver->ops->device_rem == NULL)
		return ENOTSUP;
	/* Just tell the driver to stop whatever it is doing */
	usb_device_t *usb_dev = ddf_dev_data_get(gen_dev);
	const int ret = driver->ops->device_rem(usb_dev);
	if (ret != EOK)
		return ret;
	usb_device_deinit(usb_dev);
	return EOK;
}

/** Callback when a device was removed from the system.
 *
 * This callback is a wrapper for USB specific version of @c device_gone.
 *
 * @param gen_dev Device structure as prepared by DDF.
 * @return Error code.
 */
int generic_device_gone(ddf_dev_t *gen_dev)
{
	assert(driver);
	assert(driver->ops);
	if (driver->ops->device_gone == NULL)
		return ENOTSUP;
	usb_device_t *usb_dev = ddf_dev_data_get(gen_dev);
	const int ret = driver->ops->device_gone(usb_dev);
	if (ret == EOK)
		usb_device_deinit(usb_dev);

	return ret;
}

/** Destroy existing pipes of a USB device.
 *
 * @param dev Device where to destroy the pipes.
 */
static void destroy_current_pipes(usb_device_t *dev)
{
	usb_device_destroy_pipes(dev->pipes, dev->pipes_count);
	dev->pipes = NULL;
	dev->pipes_count = 0;
}

/** Change interface setting of a device.
 * This function selects new alternate setting of an interface by issuing
 * proper USB command to the device and also creates new USB pipes
 * under @c dev->pipes.
 *
 * @warning This function is intended for drivers working at interface level.
 * For drivers controlling the whole device, you need to change interface
 * manually using usb_request_set_interface() and creating new pipes
 * with usb_pipe_initialize_from_configuration().
 *
 * @warning This is a wrapper function that does several operations that
 * can fail and that cannot be rollbacked easily. That means that a failure
 * during the SET_INTERFACE request would result in having a device with
 * no pipes at all (except the default control one). That is because the old
 * pipes needs to be unregistered at HC first and the new ones could not
 * be created.
 *
 * @param dev USB device.
 * @param alternate_setting Alternate setting to choose.
 * @param endpoints New endpoint descriptions.
 * @return Error code.
 */
int usb_device_select_interface(usb_device_t *dev, uint8_t alternate_setting,
    const usb_endpoint_description_t **endpoints)
{
	if (dev->interface_no < 0) {
		return EINVAL;
	}

	/* Destroy existing pipes. */
	destroy_current_pipes(dev);

	/* Change the interface itself. */
	int rc = usb_request_set_interface(&dev->ctrl_pipe, dev->interface_no,
	    alternate_setting);
	if (rc != EOK) {
		return rc;
	}

	/* Create new pipes. */
	rc = usb_device_create_pipes(&dev->wire, endpoints,
	    dev->descriptors.configuration, dev->descriptors.configuration_size,
	    dev->interface_no, (int)alternate_setting,
	    &dev->pipes, &dev->pipes_count);

	return rc;
}

/** Retrieve basic descriptors from the device.
 *
 * @param[in] ctrl_pipe Control endpoint pipe.
 * @param[out] descriptors Where to store the descriptors.
 * @return Error code.
 */
int usb_device_retrieve_descriptors(usb_pipe_t *ctrl_pipe,
    usb_device_descriptors_t *descriptors)
{
	assert(descriptors != NULL);

	descriptors->configuration = NULL;

	int rc;

	/* It is worth to start a long transfer. */
	usb_pipe_start_long_transfer(ctrl_pipe);

	/* Get the device descriptor. */
	rc = usb_request_get_device_descriptor(ctrl_pipe, &descriptors->device);
	if (rc != EOK) {
		goto leave;
	}

	/* Get the full configuration descriptor. */
	rc = usb_request_get_full_configuration_descriptor_alloc(
	    ctrl_pipe, 0, (void **) &descriptors->configuration,
	    &descriptors->configuration_size);

leave:
	usb_pipe_end_long_transfer(ctrl_pipe);

	return rc;
}

/** Cleanup structure initialized via usb_device_retrieve_descriptors.
 *
 * @param[in] descriptors Where to store the descriptors.
 */
void usb_device_release_descriptors(usb_device_descriptors_t *descriptors)
{
	assert(descriptors);
	free(descriptors->configuration);
	descriptors->configuration = NULL;
}

/** Create pipes for a device.
 *
 * This is more or less a wrapper that does following actions:
 * - allocate and initialize pipes
 * - map endpoints to the pipes based on the descriptions
 * - registers endpoints with the host controller
 *
 * @param[in] wire Initialized backing connection to the host controller.
 * @param[in] endpoints Endpoints description, NULL terminated.
 * @param[in] config_descr Configuration descriptor of active configuration.
 * @param[in] config_descr_size Size of @p config_descr in bytes.
 * @param[in] interface_no Interface to map from.
 * @param[in] interface_setting Interface setting (default is usually 0).
 * @param[out] pipes_ptr Where to store array of created pipes
 *	(not NULL terminated).
 * @param[out] pipes_count_ptr Where to store number of pipes
 *	(set to NULL if you wish to ignore the count).
 * @return Error code.
 */
int usb_device_create_pipes(usb_device_connection_t *wire,
    const usb_endpoint_description_t **endpoints,
    const uint8_t *config_descr, size_t config_descr_size,
    int interface_no, int interface_setting,
    usb_endpoint_mapping_t **pipes_ptr, size_t *pipes_count_ptr)
{
	assert(wire != NULL);
	assert(config_descr != NULL);
	assert(config_descr_size > 0);
	assert(pipes_ptr != NULL);

	size_t i;
	int rc;

	const size_t pipe_count = count_other_pipes(endpoints);
	if (pipe_count == 0) {
		if (pipes_count_ptr)
			*pipes_count_ptr = pipe_count;
		*pipes_ptr = NULL;
		return EOK;
	}

	usb_endpoint_mapping_t *pipes
	    = calloc(pipe_count, sizeof(usb_endpoint_mapping_t));
	if (pipes == NULL) {
		return ENOMEM;
	}

	/* Now initialize. */
	for (i = 0; i < pipe_count; i++) {
		pipes[i].description = endpoints[i];
		pipes[i].interface_no = interface_no;
		pipes[i].interface_setting = interface_setting;
	}

	/* Find the mapping from configuration descriptor. */
	rc = usb_pipe_initialize_from_configuration(pipes, pipe_count,
	    config_descr, config_descr_size, wire);
	if (rc != EOK) {
		free(pipes);
		return rc;
	}

	/* Register created pipes. */
	for (i = 0; i < pipe_count; i++) {
		if (pipes[i].present) {
			rc = usb_pipe_register(&pipes[i].pipe,
			    pipes[i].descriptor->poll_interval);
			if (rc != EOK) {
				goto rollback_unregister_endpoints;
			}
		}
	}

	*pipes_ptr = pipes;
	if (pipes_count_ptr != NULL) {
		*pipes_count_ptr = pipe_count;
	}

	return EOK;

	/*
	 * Jump here if something went wrong after endpoints have
	 * been registered.
	 * This is also the target when the registration of
	 * endpoints fails.
	 */
rollback_unregister_endpoints:
	for (i = 0; i < pipe_count; i++) {
		if (pipes[i].present) {
			usb_pipe_unregister(&pipes[i].pipe);
		}
	}

	free(pipes);
	return rc;
}

/** Destroy pipes previously created by usb_device_create_pipes.
 *
 * @param[in] pipes Endpoint mapping to be destroyed.
 * @param[in] pipes_count Number of endpoints.
 */
void usb_device_destroy_pipes(usb_endpoint_mapping_t *pipes, size_t pipes_count)
{
	/* Destroy the pipes. */
	for (size_t i = 0; i < pipes_count; ++i) {
		assert(pipes);
		usb_log_debug2("Unregistering pipe %zu: %spresent.\n",
		    i, pipes[i].present ? "" : "not ");
		if (pipes[i].present)
			usb_pipe_unregister(&pipes[i].pipe);
	}
	free(pipes);
}

/** Initialize new instance of USB device.
 *
 * @param[in] usb_dev Pointer to the new device.
 * @param[in] ddf_dev Generic DDF device backing the USB one.
 * @param[in] endpoints NULL terminated array of endpoints (NULL for none).
 * @param[out] errstr_ptr Where to store description of context
 *	(in case error occurs).
 * @return Error code.
 */
int usb_device_init(usb_device_t *usb_dev, ddf_dev_t *ddf_dev,
    const usb_endpoint_description_t **endpoints, const char **errstr_ptr)
{
	assert(usb_dev != NULL);
	assert(ddf_dev != NULL);

	*errstr_ptr = NULL;

	usb_dev->ddf_dev = ddf_dev;
	usb_dev->driver_data = NULL;
	usb_dev->descriptors.configuration = NULL;
	usb_dev->pipes_count = 0;
	usb_dev->pipes = NULL;

	/* Get assigned params */
	devman_handle_t hc_handle;
	usb_address_t address;

	int rc = usb_get_info_by_handle(ddf_dev_get_handle(ddf_dev),
	    &hc_handle, &address, &usb_dev->interface_no);
	if (rc != EOK) {
		*errstr_ptr = "device parameters retrieval";
		return rc;
	}

	/* Initialize hc connection. */
	usb_hc_connection_initialize(&usb_dev->hc_conn, hc_handle);

	/* Initialize backing wire and control pipe. */
	rc = usb_device_connection_initialize(
	    &usb_dev->wire, &usb_dev->hc_conn, address);
	if (rc != EOK) {
		*errstr_ptr = "device connection initialization";
		return rc;
	}

	/* This pipe was registered by the hub driver,
	 * during device initialization. */
	rc = usb_pipe_initialize_default_control(
	    &usb_dev->ctrl_pipe, &usb_dev->wire);
	if (rc != EOK) {
		*errstr_ptr = "default control pipe initialization";
		return rc;
	}

	/* Open hc connection for pipe registration. */
	rc = usb_hc_connection_open(&usb_dev->hc_conn);
	if (rc != EOK) {
		*errstr_ptr = "hc connection open";
		return rc;
	}

	/* Retrieve standard descriptors. */
	rc = usb_device_retrieve_descriptors(
	    &usb_dev->ctrl_pipe, &usb_dev->descriptors);
	if (rc != EOK) {
		*errstr_ptr = "descriptor retrieval";
		usb_hc_connection_close(&usb_dev->hc_conn);
		return rc;
	}

	/* Create alternate interfaces. We will silently ignore failure.
	 * We might either control one interface or an entire device,
	 * it makes no sense to speak about alternate interfaces when
	 * controlling a device. */
	rc = usb_alternate_interfaces_init(&usb_dev->alternate_interfaces,
	    usb_dev->descriptors.configuration,
	    usb_dev->descriptors.configuration_size, usb_dev->interface_no);
	const int alternate_iface =
	    (rc == EOK) ? usb_dev->alternate_interfaces.current : 0;

	/* Create and register other pipes than default control (EP 0) */
	rc = usb_device_create_pipes(&usb_dev->wire, endpoints,
	    usb_dev->descriptors.configuration,
	    usb_dev->descriptors.configuration_size,
	    usb_dev->interface_no, (int)alternate_iface,
	    &usb_dev->pipes, &usb_dev->pipes_count);
	if (rc != EOK) {
		usb_hc_connection_close(&usb_dev->hc_conn);
		/* Full configuration descriptor is allocated. */
		usb_device_release_descriptors(&usb_dev->descriptors);
		/* Alternate interfaces may be allocated */
		usb_alternate_interfaces_deinit(&usb_dev->alternate_interfaces);
		*errstr_ptr = "pipes initialization";
		return rc;
	}

	usb_hc_connection_close(&usb_dev->hc_conn);
	return EOK;
}

/** Clean instance of a USB device.
 *
 * @param dev Device to be de-initialized.
 *
 * Does not free/destroy supplied pointer.
 */
void usb_device_deinit(usb_device_t *dev)
{
	if (dev) {
		/* Destroy existing pipes. */
		destroy_current_pipes(dev);
		/* Ignore errors and hope for the best. */
		usb_hc_connection_deinitialize(&dev->hc_conn);
		usb_alternate_interfaces_deinit(&dev->alternate_interfaces);
		usb_device_release_descriptors(&dev->descriptors);
		free(dev->driver_data);
		dev->driver_data = NULL;
	}
}

/** Allocate driver specific data.
 * @param usb_dev usb_device structure.
 * @param size requested data size.
 * @return Pointer to the newly allocated space, NULL on failure.
 */
void * usb_device_data_alloc(usb_device_t *usb_dev, size_t size)
{
	assert(usb_dev);
	assert(usb_dev->driver_data == NULL);
	return usb_dev->driver_data = calloc(1, size);

}

/**
 * @}
 */
