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
    usb_device_t *dev)
{
	int rc;

	size_t pipe_count = count_other_pipes(endpoints);
	if (pipe_count == 0) {
		return EOK;
	}

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
		dev->pipes[i].pipe = malloc(sizeof(usb_pipe_t));
		if (dev->pipes[i].pipe == NULL) {
			usb_log_oom(dev->ddf_dev);
			rc = ENOMEM;
			goto rollback;
		}

		dev->pipes[i].description = endpoints[i];
		dev->pipes[i].interface_no = dev->interface_no;
		dev->pipes[i].interface_setting = 0;
	}

	rc = usb_pipe_initialize_from_configuration(dev->pipes, pipe_count,
	    dev->descriptors.configuration, dev->descriptors.configuration_size,
	    &dev->wire);
	if (rc != EOK) {
		usb_log_error("Failed initializing USB endpoints: %s.\n",
		    str_error(rc));
		goto rollback;
	}

	/* Register the endpoints. */
	usb_hc_connection_t hc_conn;
	rc = usb_hc_connection_initialize_from_device(&hc_conn, dev->ddf_dev);
	if (rc != EOK) {
		usb_log_error(
		    "Failed initializing connection to host controller: %s.\n",
		    str_error(rc));
		goto rollback;
	}
	rc = usb_hc_connection_open(&hc_conn);
	if (rc != EOK) {
		usb_log_error("Failed to connect to host controller: %s.\n",
		    str_error(rc));
		goto rollback;
	}
	for (i = 0; i < pipe_count; i++) {
		if (dev->pipes[i].present) {
			rc = usb_pipe_register(dev->pipes[i].pipe,
			    dev->pipes[i].descriptor->poll_interval,
			    &hc_conn);
			/* Ignore error when operation not supported by HC. */
			if ((rc != EOK) && (rc != ENOTSUP)) {
				/* FIXME: what shall we do? */
				dev->pipes[i].present = false;
				free(dev->pipes[i].pipe);
				dev->pipes[i].pipe = NULL;
			}
		}
	}
	/* Ignoring errors here. */
	usb_hc_connection_close(&hc_conn);

	dev->pipes_count = pipe_count;

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
	 * For further actions, we need open session on default control pipe.
	 */
	rc = usb_pipe_start_session(&dev->ctrl_pipe);
	if (rc != EOK) {
		usb_log_error("Failed to start an IPC session: %s.\n",
		    str_error(rc));
		return rc;
	}

	/* Get the device descriptor. */
	rc = usb_request_get_device_descriptor(&dev->ctrl_pipe,
	    &dev->descriptors.device);
	if (rc != EOK) {
		usb_log_error("Failed to retrieve device descriptor: %s.\n",
		    str_error(rc));
		return rc;
	}

	/* Get the full configuration descriptor. */
	rc = usb_request_get_full_configuration_descriptor_alloc(
	    &dev->ctrl_pipe, 0, (void **) &dev->descriptors.configuration,
	    &dev->descriptors.configuration_size);
	if (rc != EOK) {
		usb_log_error("Failed retrieving configuration descriptor: %s. %s\n",
		    dev->ddf_dev->name, str_error(rc));
		return rc;
	}

	if (driver->endpoints != NULL) {
		rc = initialize_other_pipes(driver->endpoints, dev);
	}

	/* No checking here. */
	usb_pipe_end_session(&dev->ctrl_pipe);

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
static size_t count_alternate_interfaces(uint8_t *config_descr,
    size_t config_descr_size, int interface_no)
{
	assert(config_descr != NULL);
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
	    = count_alternate_interfaces(dev->descriptors.configuration,
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
	size_t i;
	int rc;

	/* TODO: this shall be done under some device mutex. */

	/* First check that no session is opened. */
	for (i = 0; i < dev->pipes_count; i++) {
		if (usb_pipe_is_session_started(dev->pipes[i].pipe)) {
			return EBUSY;
		}
	}

	/* Prepare connection to HC. */
	usb_hc_connection_t hc_conn;
	rc = usb_hc_connection_initialize_from_device(&hc_conn, dev->ddf_dev);
	if (rc != EOK) {
		return rc;
	}
	rc = usb_hc_connection_open(&hc_conn);
	if (rc != EOK) {
		return rc;
	}

	/* Destroy the pipes. */
	for (i = 0; i < dev->pipes_count; i++) {
		usb_pipe_unregister(dev->pipes[i].pipe, &hc_conn);
		free(dev->pipes[i].pipe);
	}

	usb_hc_connection_close(&hc_conn);

	free(dev->pipes);
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
	rc = initialize_other_pipes(endpoints, dev);

	return rc;
}

/**
 * @}
 */
