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
#include <usb/dp.h>
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

/** Count number of pipes the driver expects.
 *
 * @param drv USB driver.
 * @return Number of pipes (excluding default control pipe).
 */
static size_t count_other_pipes(usb_endpoint_description_t **endpoints)
{
	size_t count = 0;
	if (endpoints == NULL) {
		return 0;
	}

	while (endpoints[count] != NULL) {
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
static int initialize_other_pipes(usb_endpoint_description_t **endpoints,
    usb_device_t *dev, int alternate_setting)
{
	usb_endpoint_mapping_t *pipes;
	size_t pipes_count;

	int rc = usb_device_create_pipes(dev->ddf_dev, &dev->wire, endpoints,
	    dev->descriptors.configuration, dev->descriptors.configuration_size,
	    dev->interface_no, alternate_setting,
	    &pipes, &pipes_count);

	if (rc != EOK) {
		usb_log_error(
		    "Failed to create endpoint pipes for `%s': %s.\n",
		    dev->ddf_dev->name, str_error(rc));
		return rc;
	}

	dev->pipes = pipes;
	dev->pipes_count = pipes_count;

	return EOK;
}

/** Initialize all endpoint pipes.
 *
 * @param drv The driver.
 * @param dev The device to be initialized.
 * @return Error code.
 */
static int initialize_pipes(usb_device_t *dev)
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

	rc = usb_pipe_initialize_default_control(&dev->ctrl_pipe,
	    &dev->wire);
	if (rc != EOK) {
		usb_log_error("Failed to initialize default control pipe " \
		    "on device `%s': %s.\n",
		    dev->ddf_dev->name, str_error(rc));
		return rc;
	}

	rc = usb_pipe_probe_default_control(&dev->ctrl_pipe);
	if (rc != EOK) {
		usb_log_error(
		    "Probing default control pipe on device `%s' failed: %s.\n",
		    dev->ddf_dev->name, str_error(rc));
		return rc;
	}

	/* Get our interface. */
	dev->interface_no = usb_device_get_assigned_interface(dev->ddf_dev);

	/*
	 * We will do some querying of the device, it is worth to prepare
	 * the long transfer.
	 */
	rc = usb_pipe_start_long_transfer(&dev->ctrl_pipe);
	if (rc != EOK) {
		usb_log_error("Failed to start transfer: %s.\n",
		    str_error(rc));
		return rc;
	}

	/* Retrieve the descriptors. */
	rc = usb_device_retrieve_descriptors(&dev->ctrl_pipe,
	    &dev->descriptors);
	if (rc != EOK) {
		usb_log_error("Failed to retrieve standard device " \
		    "descriptors of %s: %s.\n",
		    dev->ddf_dev->name, str_error(rc));
		return rc;
	}


	if (driver->endpoints != NULL) {
		rc = initialize_other_pipes(driver->endpoints, dev, 0);
	}

	usb_pipe_end_long_transfer(&dev->ctrl_pipe);

	/* Rollback actions. */
	if (rc != EOK) {
		if (dev->descriptors.configuration != NULL) {
			free(dev->descriptors.configuration);
		}
	}

	return rc;
}

/** Count number of alternate settings of a interface.
 *
 * @param config_descr Full configuration descriptor.
 * @param config_descr_size Size of @p config_descr in bytes.
 * @param interface_no Interface number.
 * @return Number of alternate interfaces for @p interface_no interface.
 */
size_t usb_interface_count_alternates(uint8_t *config_descr,
    size_t config_descr_size, uint8_t interface_no)
{
	assert(config_descr != NULL);
	assert(config_descr_size > 0);

	usb_dp_parser_t dp_parser = {
		.nesting = usb_dp_standard_descriptor_nesting
	};
	usb_dp_parser_data_t dp_data = {
		.data = config_descr,
		.size = config_descr_size,
		.arg = NULL
	};

	size_t alternate_count = 0;

	uint8_t *iface_ptr = usb_dp_get_nested_descriptor(&dp_parser,
	    &dp_data, config_descr);
	while (iface_ptr != NULL) {
		usb_standard_interface_descriptor_t *iface
		    = (usb_standard_interface_descriptor_t *) iface_ptr;
		if (iface->descriptor_type == USB_DESCTYPE_INTERFACE) {
			if (iface->interface_number == interface_no) {
				alternate_count++;
			}
		}
		iface_ptr = usb_dp_get_sibling_descriptor(&dp_parser, &dp_data,
		    config_descr, iface_ptr);
	}

	return alternate_count;
}

/** Initialize structures related to alternate interfaces.
 *
 * @param dev Device where alternate settings shall be initialized.
 * @return Error code.
 */
static int initialize_alternate_interfaces(usb_device_t *dev)
{
	if (dev->interface_no < 0) {
		dev->alternate_interfaces = NULL;
		return EOK;
	}

	usb_alternate_interfaces_t *alternates
	    = malloc(sizeof(usb_alternate_interfaces_t));

	if (alternates == NULL) {
		return ENOMEM;
	}

	alternates->alternative_count
	    = usb_interface_count_alternates(dev->descriptors.configuration,
	    dev->descriptors.configuration_size, dev->interface_no);

	if (alternates->alternative_count == 0) {
		free(alternates);
		return ENOENT;
	}

	alternates->alternatives = malloc(alternates->alternative_count
	    * sizeof(usb_alternate_interface_descriptors_t));
	if (alternates->alternatives == NULL) {
		free(alternates);
		return ENOMEM;
	}

	alternates->current = 0;

	usb_dp_parser_t dp_parser = {
		.nesting = usb_dp_standard_descriptor_nesting
	};
	usb_dp_parser_data_t dp_data = {
		.data = dev->descriptors.configuration,
		.size = dev->descriptors.configuration_size,
		.arg = NULL
	};

	usb_alternate_interface_descriptors_t *cur_alt_iface
	    = &alternates->alternatives[0];

	uint8_t *iface_ptr = usb_dp_get_nested_descriptor(&dp_parser,
	    &dp_data, dp_data.data);
	while (iface_ptr != NULL) {
		usb_standard_interface_descriptor_t *iface
		    = (usb_standard_interface_descriptor_t *) iface_ptr;
		if ((iface->descriptor_type != USB_DESCTYPE_INTERFACE)
		    || (iface->interface_number != dev->interface_no)) {
			iface_ptr = usb_dp_get_sibling_descriptor(&dp_parser,
			    &dp_data,
			    dp_data.data, iface_ptr);
			continue;
		}

		cur_alt_iface->interface = iface;
		cur_alt_iface->nested_descriptors = iface_ptr + sizeof(*iface);

		/* Find next interface to count size of nested descriptors. */
		iface_ptr = usb_dp_get_sibling_descriptor(&dp_parser, &dp_data,
		    dp_data.data, iface_ptr);
		if (iface_ptr == NULL) {
			uint8_t *next = dp_data.data + dp_data.size;
			cur_alt_iface->nested_descriptors_size
			    = next - cur_alt_iface->nested_descriptors;
		} else {
			cur_alt_iface->nested_descriptors_size
			    = iface_ptr - cur_alt_iface->nested_descriptors;
		}

		cur_alt_iface++;
	}

	dev->alternate_interfaces = alternates;

	return EOK;
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
	dev->descriptors.configuration = NULL;

	dev->pipes_count = 0;
	dev->pipes = NULL;

	rc = initialize_pipes(dev);
	if (rc != EOK) {
		free(dev);
		return rc;
	}

	(void) initialize_alternate_interfaces(dev);

	return driver->ops->add_device(dev);
}

/** Destroy existing pipes of a USB device.
 *
 * @param dev Device where to destroy the pipes.
 * @return Error code.
 */
static int destroy_current_pipes(usb_device_t *dev)
{
	int rc = usb_device_destroy_pipes(dev->ddf_dev,
	    dev->pipes, dev->pipes_count);
	if (rc != EOK) {
		return rc;
	}

	dev->pipes = NULL;
	dev->pipes_count = 0;

	return EOK;
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
 * @param dev USB device.
 * @param alternate_setting Alternate setting to choose.
 * @param endpoints New endpoint descriptions.
 * @return Error code.
 */
int usb_device_select_interface(usb_device_t *dev, uint8_t alternate_setting,
    usb_endpoint_description_t **endpoints)
{
	if (dev->interface_no < 0) {
		return EINVAL;
	}

	int rc;

	/* TODO: more transactional behavior. */

	/* Destroy existing pipes. */
	rc = destroy_current_pipes(dev);
	if (rc != EOK) {
		return rc;
	}

	/* Change the interface itself. */
	rc = usb_request_set_interface(&dev->ctrl_pipe, dev->interface_no,
	    alternate_setting);
	if (rc != EOK) {
		return rc;
	}

	/* Create new pipes. */
	rc = initialize_other_pipes(endpoints, dev, (int) alternate_setting);

	return rc;
}

/** Retrieve basic descriptors from the device.
 *
 * @param[in] ctrl_pipe Control pipe with opened session.
 * @param[out] descriptors Where to store the descriptors.
 * @return Error code.
 */
int usb_device_retrieve_descriptors(usb_pipe_t *ctrl_pipe,
    usb_device_descriptors_t *descriptors)
{
	assert(descriptors != NULL);
	assert(usb_pipe_is_session_started(ctrl_pipe));

	descriptors->configuration = NULL;

	int rc;

	/* Get the device descriptor. */
	rc = usb_request_get_device_descriptor(ctrl_pipe, &descriptors->device);
	if (rc != EOK) {
		return rc;
	}

	/* Get the full configuration descriptor. */
	rc = usb_request_get_full_configuration_descriptor_alloc(
	    ctrl_pipe, 0, (void **) &descriptors->configuration,
	    &descriptors->configuration_size);
	if (rc != EOK) {
		return rc;
	}

	return EOK;
}

/** Create pipes for a device.
 *
 * This is more or less a wrapper that does following actions:
 * - allocate and initialize pipes
 * - map endpoints to the pipes based on the descriptions
 * - registers endpoints with the host controller
 *
 * @param[in] dev Generic DDF device backing the USB one.
 * @param[in] wire Initialized backing connection to the host controller.
 * @param[in] endpoints Endpoints description, NULL terminated.
 * @param[in] config_descr Configuration descriptor of active configuration.
 * @param[in] config_descr_size Size of @p config_descr in bytes.
 * @param[in] interface_no Interface to map from.
 * @param[in] interface_setting Interface setting (default is usually 0).
 * @param[out] pipes_ptr Where to store array of created pipes
 *	(not NULL terminated).
 * @param[out] pipes_count_ptr Where to store number of pipes
 *	(set to if you wish to ignore the count).
 * @return Error code.
 */
int usb_device_create_pipes(ddf_dev_t *dev, usb_device_connection_t *wire,
    usb_endpoint_description_t **endpoints,
    uint8_t *config_descr, size_t config_descr_size,
    int interface_no, int interface_setting,
    usb_endpoint_mapping_t **pipes_ptr, size_t *pipes_count_ptr)
{
	assert(dev != NULL);
	assert(wire != NULL);
	assert(endpoints != NULL);
	assert(config_descr != NULL);
	assert(config_descr_size > 0);
	assert(pipes_ptr != NULL);

	size_t i;
	int rc;

	size_t pipe_count = count_other_pipes(endpoints);
	if (pipe_count == 0) {
		*pipes_ptr = NULL;
		return EOK;
	}

	usb_endpoint_mapping_t *pipes
	    = malloc(sizeof(usb_endpoint_mapping_t) * pipe_count);
	if (pipes == NULL) {
		return ENOMEM;
	}

	/* Initialize to NULL to allow smooth rollback. */
	for (i = 0; i < pipe_count; i++) {
		pipes[i].pipe = NULL;
	}

	/* Now allocate and fully initialize. */
	for (i = 0; i < pipe_count; i++) {
		pipes[i].pipe = malloc(sizeof(usb_pipe_t));
		if (pipes[i].pipe == NULL) {
			rc = ENOMEM;
			goto rollback_free_only;
		}
		pipes[i].description = endpoints[i];
		pipes[i].interface_no = interface_no;
		pipes[i].interface_setting = interface_setting;
	}

	/* Find the mapping from configuration descriptor. */
	rc = usb_pipe_initialize_from_configuration(pipes, pipe_count,
	    config_descr, config_descr_size, wire);
	if (rc != EOK) {
		goto rollback_free_only;
	}

	/* Register the endpoints with HC. */
	usb_hc_connection_t hc_conn;
	rc = usb_hc_connection_initialize_from_device(&hc_conn, dev);
	if (rc != EOK) {
		goto rollback_free_only;
	}

	rc = usb_hc_connection_open(&hc_conn);
	if (rc != EOK) {
		goto rollback_free_only;
	}

	for (i = 0; i < pipe_count; i++) {
		if (pipes[i].present) {
			rc = usb_pipe_register(pipes[i].pipe,
			    pipes[i].descriptor->poll_interval, &hc_conn);
			if (rc != EOK) {
				goto rollback_unregister_endpoints;
			}
		}
	}

	usb_hc_connection_close(&hc_conn);

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
			usb_pipe_unregister(pipes[i].pipe, &hc_conn);
		}
	}

	usb_hc_connection_close(&hc_conn);

	/*
	 * Jump here if something went wrong before some actual communication
	 * with HC. Then the only thing that needs to be done is to free
	 * allocated memory.
	 */
rollback_free_only:
	for (i = 0; i < pipe_count; i++) {
		if (pipes[i].pipe != NULL) {
			free(pipes[i].pipe);
		}
	}
	free(pipes);

	return rc;
}

/** Destroy pipes previously created by usb_device_create_pipes.
 *
 * @param[in] dev Generic DDF device backing the USB one.
 * @param[in] pipes Endpoint mapping to be destroyed.
 * @param[in] pipes_count Number of endpoints.
 */
int usb_device_destroy_pipes(ddf_dev_t *dev,
    usb_endpoint_mapping_t *pipes, size_t pipes_count)
{
	assert(dev != NULL);
	assert(((pipes != NULL) && (pipes_count > 0))
	    || ((pipes == NULL) && (pipes_count == 0)));

	if (pipes_count == 0) {
		return EOK;
	}

	int rc;

	/* Prepare connection to HC to allow endpoint unregistering. */
	usb_hc_connection_t hc_conn;
	rc = usb_hc_connection_initialize_from_device(&hc_conn, dev);
	if (rc != EOK) {
		return rc;
	}
	rc = usb_hc_connection_open(&hc_conn);
	if (rc != EOK) {
		return rc;
	}

	/* Destroy the pipes. */
	size_t i;
	for (i = 0; i < pipes_count; i++) {
		usb_pipe_unregister(pipes[i].pipe, &hc_conn);
		free(pipes[i].pipe);
	}

	usb_hc_connection_close(&hc_conn);

	free(pipes);

	return EOK;
}

/**
 * @}
 */
