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

/** USB device structure. */
typedef struct usb_device {
	/** Connection to USB hc, used by wire and arbitrary requests. */
	usb_hc_connection_t hc_conn;
	/** Connection backing the pipes.
	 * Typically, you will not need to use this attribute at all.
	 */
	usb_device_connection_t wire;
	/** The default control pipe. */
	usb_pipe_t ctrl_pipe;

	/** Other endpoint pipes.
	 * This is an array of other endpoint pipes in the same order as
	 * in usb_driver_t.
	 */
	usb_endpoint_mapping_t *pipes;
	/** Number of other endpoint pipes. */
	size_t pipes_count;
	/** Current interface.
	 * Usually, drivers operate on single interface only.
	 * This item contains the value of the interface or -1 for any.
	 */
	int interface_no;
	/** Alternative interfaces. */
	usb_alternate_interfaces_t alternate_interfaces;

	/** Some useful descriptors for USB device. */
	struct {
		/** Standard device descriptor. */
		usb_standard_device_descriptor_t device;
		/** Full configuration descriptor of current configuration. */
		const uint8_t *configuration;
		size_t configuration_size;
	} descriptors;

	/** Generic DDF device backing this one. DO NOT TOUCH! */
	ddf_dev_t *ddf_dev;
	/** Custom driver data.
	 * Do not use the entry in generic device, that is already used
	 * by the framework.
	 */
	void *driver_data;

	usb_dev_session_t *bus_session;
} usb_device_t;

/** Count number of pipes the driver expects.
 *
 * @param drv USB driver.
 * @return Number of pipes (excluding default control pipe).
 */
static inline size_t count_pipes(const usb_endpoint_description_t **endpoints)
{
	size_t count;
	for (count = 0; endpoints != NULL && endpoints[count] != NULL; ++count);
	return count;
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
int usb_device_select_interface(usb_device_t *usb_dev,
    uint8_t alternate_setting, const usb_endpoint_description_t **endpoints)
{
	assert(usb_dev);

	if (usb_dev->interface_no < 0) {
		return EINVAL;
	}

	/* Change the interface itself. */
	int rc = usb_request_set_interface(&usb_dev->ctrl_pipe,
	    usb_dev->interface_no, alternate_setting);
	if (rc != EOK) {
		return rc;
	}

	/* Change current alternative */
	usb_dev->alternate_interfaces.current = alternate_setting;

	/* Destroy existing pipes. */
	usb_device_destroy_pipes(usb_dev);

	/* Create new pipes. */
	rc = usb_device_create_pipes(usb_dev, endpoints);

	return rc;
}

/** Retrieve basic descriptors from the device.
 *
 * @param[in] ctrl_pipe Control endpoint pipe.
 * @param[out] descriptors Where to store the descriptors.
 * @return Error code.
 */
static int usb_device_retrieve_descriptors(usb_device_t *usb_dev)
{
	assert(usb_dev);
	assert(usb_dev->descriptors.configuration == NULL);

	/* It is worth to start a long transfer. */
	usb_pipe_start_long_transfer(&usb_dev->ctrl_pipe);

	/* Get the device descriptor. */
	int rc = usb_request_get_device_descriptor(&usb_dev->ctrl_pipe,
	    &usb_dev->descriptors.device);
	if (rc != EOK) {
		goto leave;
	}

	/* Get the full configuration descriptor. */
	rc = usb_request_get_full_configuration_descriptor_alloc(
	    &usb_dev->ctrl_pipe, 0,
	    (void **) &usb_dev->descriptors.configuration,
	    &usb_dev->descriptors.configuration_size);

leave:
	usb_pipe_end_long_transfer(&usb_dev->ctrl_pipe);

	return rc;
}

/** Cleanup structure initialized via usb_device_retrieve_descriptors.
 *
 * @param[in] descriptors Where to store the descriptors.
 */
static void usb_device_release_descriptors(usb_device_t *usb_dev)
{
	assert(usb_dev);
	free(usb_dev->descriptors.configuration);
	usb_dev->descriptors.configuration = NULL;
	usb_dev->descriptors.configuration_size = 0;
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
int usb_device_create_pipes(usb_device_t *usb_dev,
    const usb_endpoint_description_t **endpoints)
{
	assert(usb_dev);
	assert(usb_dev->descriptors.configuration);
	assert(usb_dev->pipes == NULL);
	assert(usb_dev->pipes_count == 0);

	size_t pipe_count = count_pipes(endpoints);
	if (pipe_count == 0) {
		return EOK;
	}

	usb_endpoint_mapping_t *pipes =
	    calloc(pipe_count, sizeof(usb_endpoint_mapping_t));
	if (pipes == NULL) {
		return ENOMEM;
	}

	/* Now initialize. */
	for (size_t i = 0; i < pipe_count; i++) {
		pipes[i].description = endpoints[i];
		pipes[i].interface_no = usb_dev->interface_no;
		pipes[i].interface_setting =
		    usb_dev->alternate_interfaces.current;
	}

	/* Find the mapping from configuration descriptor. */
	int rc = usb_pipe_initialize_from_configuration(pipes, pipe_count,
	    usb_dev->descriptors.configuration,
	    usb_dev->descriptors.configuration_size, &usb_dev->wire);
	if (rc != EOK) {
		free(pipes);
		return rc;
	}

	/* Register created pipes. */
	for (size_t i = 0; i < pipe_count; i++) {
		if (pipes[i].present) {
			rc = usb_pipe_register(&pipes[i].pipe,
			    pipes[i].descriptor->poll_interval);
			if (rc != EOK) {
				goto rollback_unregister_endpoints;
			}
		}
	}

	usb_dev->pipes = pipes;
	usb_dev->pipes_count = pipe_count;

	return EOK;

	/*
	 * Jump here if something went wrong after endpoints have
	 * been registered.
	 * This is also the target when the registration of
	 * endpoints fails.
	 */
rollback_unregister_endpoints:
	for (size_t i = 0; i < pipe_count; i++) {
		if (pipes[i].present) {
			usb_pipe_unregister(&pipes[i].pipe);
		}
	}

	free(pipes);
	return rc;
}

/** Destroy pipes previously created by usb_device_create_pipes.
 *
 * @param[in] usb_dev USB device.
 */
void usb_device_destroy_pipes(usb_device_t *usb_dev)
{
	assert(usb_dev);
	assert(usb_dev->pipes || usb_dev->pipes_count == 0);
	/* Destroy the pipes. */
	for (size_t i = 0; i < usb_dev->pipes_count; ++i) {
		usb_log_debug2("Unregistering pipe %zu: %spresent.\n",
		    i, usb_dev->pipes[i].present ? "" : "not ");
		if (usb_dev->pipes[i].present)
			usb_pipe_unregister(&usb_dev->pipes[i].pipe);
	}
	free(usb_dev->pipes);
	usb_dev->pipes = NULL;
	usb_dev->pipes_count = 0;
}

usb_pipe_t *usb_device_get_default_pipe(usb_device_t *usb_dev)
{
	assert(usb_dev);
	return &usb_dev->ctrl_pipe;
}

usb_endpoint_mapping_t *usb_device_get_mapped_ep_desc(usb_device_t *usb_dev,
    const usb_endpoint_description_t *desc)
{
	assert(usb_dev);
	for (unsigned i = 0; i < usb_dev->pipes_count; ++i) {
		if (usb_dev->pipes[i].description == desc)
			return &usb_dev->pipes[i];
	}
	return NULL;
}

usb_endpoint_mapping_t * usb_device_get_mapped_ep(
    usb_device_t *usb_dev, usb_endpoint_t ep)
{
	assert(usb_dev);
	for (unsigned i = 0; i < usb_dev->pipes_count; ++i) {
		if (usb_dev->pipes[i].pipe.endpoint_no == ep)
			return &usb_dev->pipes[i];
	}
	return NULL;
}

int usb_device_get_iface_number(usb_device_t *usb_dev)
{
	assert(usb_dev);
	return usb_dev->interface_no;
}

const usb_standard_device_descriptor_t *
usb_device_get_device_descriptor(usb_device_t *usb_dev)
{
	assert(usb_dev);
	return &usb_dev->descriptors.device;
}

const void * usb_device_get_configuration_descriptor(
    usb_device_t *usb_dev, size_t *size)
{
	assert(usb_dev);
	if (size)
		*size = usb_dev->descriptors.configuration_size;
	return usb_dev->descriptors.configuration;
}

const usb_alternate_interfaces_t * usb_device_get_alternative_ifaces(
    usb_device_t *usb_dev)
{
	assert(usb_dev);
	return &usb_dev->alternate_interfaces;
}

static int usb_dev_get_info(usb_device_t *usb_dev, devman_handle_t *handle,
    usb_address_t *address, int *iface_no)
{
	assert(usb_dev);

	int ret = EOK;
	async_exch_t *exch = async_exchange_begin(usb_dev->bus_session);
	if (!exch)
		ret = ENOMEM;

	if (ret == EOK && address)
		ret = usb_get_my_address(exch, address);

	if (ret == EOK && handle)
		ret = usb_get_hc_handle(exch, handle);

	if (ret == EOK && iface_no) {
		ret = usb_get_my_interface(exch, iface_no);
		if (ret == ENOTSUP) {
			ret = EOK;
			*iface_no = -1;
		}
	}

	async_exchange_end(exch);
	return ret;
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
static int usb_device_init(usb_device_t *usb_dev, ddf_dev_t *ddf_dev,
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

	usb_dev->bus_session = usb_dev_connect_to_self(ddf_dev);
	if (!usb_dev->bus_session) {
		*errstr_ptr = "device bus session create";
		return ENOMEM;
	}

	/* Get assigned params */
	devman_handle_t hc_handle;
	usb_address_t address;

	int rc = usb_dev_get_info(usb_dev,
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
	rc = usb_device_retrieve_descriptors(usb_dev);
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

	/* Create and register other pipes than default control (EP 0) */
	rc = usb_device_create_pipes(usb_dev, endpoints);
	if (rc != EOK) {
		usb_hc_connection_close(&usb_dev->hc_conn);
		/* Full configuration descriptor is allocated. */
		usb_device_release_descriptors(usb_dev);
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
static void usb_device_fini(usb_device_t *dev)
{
	if (dev) {
		usb_dev_disconnect(dev->bus_session);
		/* Destroy existing pipes. */
		usb_device_destroy_pipes(dev);
		/* Ignore errors and hope for the best. */
		usb_hc_connection_deinitialize(&dev->hc_conn);
		usb_alternate_interfaces_deinit(&dev->alternate_interfaces);
		usb_device_release_descriptors(dev);
		free(dev->driver_data);
		dev->driver_data = NULL;
	}
}

int usb_device_create_ddf(ddf_dev_t *ddf_dev,
    const usb_endpoint_description_t **desc, const char **err)
{
	assert(ddf_dev);
	assert(err);
	usb_device_t *dev = ddf_dev_data_alloc(ddf_dev, sizeof(usb_device_t));
	if (dev == NULL) {
		*err = "DDF data alloc";
		return ENOMEM;
	}
	return usb_device_init(dev, ddf_dev, desc, err);
}

void usb_device_destroy_ddf(ddf_dev_t *ddf_dev)
{
	assert(ddf_dev);
	usb_device_t *dev = ddf_dev_data_get(ddf_dev);
	assert(dev);
	usb_device_fini(dev);
	return;
}

const char *usb_device_get_name(usb_device_t *usb_dev)
{
	assert(usb_dev);
	if (usb_dev->ddf_dev)
		return ddf_dev_get_name(usb_dev->ddf_dev);
	return NULL;
}

ddf_fun_t *usb_device_ddf_fun_create(usb_device_t *usb_dev, fun_type_t ftype,
    const char* name)
{
	assert(usb_dev);
	if (usb_dev->ddf_dev)
		return ddf_fun_create(usb_dev->ddf_dev, ftype, name);
	return NULL;
}

async_exch_t * usb_device_bus_exchange_begin(usb_device_t *usb_dev)
{
	assert(usb_dev);
	return async_exchange_begin(usb_dev->bus_session);
}

void usb_device_bus_exchange_end(async_exch_t *exch)
{
	async_exchange_end(exch);
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

void * usb_device_data_get(usb_device_t *usb_dev)
{
	assert(usb_dev);
	return usb_dev->driver_data;
}
/**
 * @}
 */
