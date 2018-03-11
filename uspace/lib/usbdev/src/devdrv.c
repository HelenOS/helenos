/*
 * Copyright (c) 2011 Vojtech Horky
 * Copyright (c) 2011 Jan Vesely
 * Copyright (c) 2018 Ondrej Hlavaty, Petr Manek, Michal Staruch
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

#include <usb_iface.h>
#include <usb/dev/alternate_ifaces.h>
#include <usb/dev/device.h>
#include <usb/dev/pipes.h>
#include <usb/dev/request.h>
#include <usb/debug.h>
#include <usb/descriptor.h>
#include <usb/usb.h>

#include <assert.h>
#include <async.h>
#include <devman.h>
#include <errno.h>
#include <str_error.h>
#include <stdlib.h>

#include <ddf/driver.h>

/** USB device structure. */
struct usb_device {
	/** Connection to device on USB bus */
	usb_dev_session_t *bus_session;

	/** devman handle */
	devman_handle_t handle;

	/** The default control pipe. */
	usb_pipe_t ctrl_pipe;

	/** Other endpoint pipes.
	 *
	 * This is an array of other endpoint pipes in the same order as
	 * in usb_driver_t.
	 */
	usb_endpoint_mapping_t *pipes;

	/** Number of other endpoint pipes. */
	size_t pipes_count;

	/** USB address of this device */
	usb_address_t address;

	/** Depth in the USB hub hiearchy */
	unsigned depth;

	/** USB speed of this device */
	usb_speed_t speed;

	/** Current interface.
	 *
	 * Usually, drivers operate on single interface only.
	 * This item contains the value of the interface or -1 for any.
	 */
	int interface_no;

	/** Alternative interfaces. */
	usb_alternate_interfaces_t alternate_interfaces;

	/** Some useful descriptors for USB device. */
	usb_device_descriptors_t descriptors;

	/** Generic DDF device backing this one. DO NOT TOUCH! */
	ddf_dev_t *ddf_dev;

	/** Custom driver data.
	 *
	 * Do not use the entry in generic device, that is already used
	 * by the framework.
	 */
	void *driver_data;
};

/** Count number of pipes the driver expects.
 *
 * @param drv USB driver.
 * @return Number of pipes (excluding default control pipe).
 */
static inline size_t count_pipes(const usb_endpoint_description_t **endpoints)
{
	size_t count = 0;
	while (endpoints != NULL && endpoints[count] != NULL)
		++count;
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
errno_t usb_device_select_interface(usb_device_t *usb_dev,
    uint8_t alternate_setting, const usb_endpoint_description_t **endpoints)
{
	assert(usb_dev);

	if (usb_dev->interface_no < 0) {
		return EINVAL;
	}

	/* Change the interface itself. */
	errno_t rc = usb_request_set_interface(&usb_dev->ctrl_pipe,
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
static errno_t usb_device_retrieve_descriptors(usb_device_t *usb_dev)
{
	assert(usb_dev);
	assert(usb_dev->descriptors.full_config == NULL);

	/* Get the device descriptor. */
	errno_t rc = usb_request_get_device_descriptor(&usb_dev->ctrl_pipe,
	    &usb_dev->descriptors.device);
	if (rc != EOK) {
		return rc;
	}

	/* Get the full configuration descriptor. */
	rc = usb_request_get_full_configuration_descriptor_alloc(
	    &usb_dev->ctrl_pipe, 0,
	    &usb_dev->descriptors.full_config,
	    &usb_dev->descriptors.full_config_size);


	return rc;
}

/** Cleanup structure initialized via usb_device_retrieve_descriptors.
 *
 * @param[in] descriptors Where to store the descriptors.
 */
static void usb_device_release_descriptors(usb_device_t *usb_dev)
{
	assert(usb_dev);
	free(usb_dev->descriptors.full_config);
	usb_dev->descriptors.full_config = NULL;
	usb_dev->descriptors.full_config_size = 0;
}

/** Create pipes for a device.
 *
 * This is more or less a wrapper that does following actions:
 * - allocate and initialize pipes
 * - map endpoints to the pipes based on the descriptions
 * - registers endpoints with the host controller
 *
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
errno_t usb_device_create_pipes(usb_device_t *usb_dev,
    const usb_endpoint_description_t **endpoints)
{
	assert(usb_dev);
	assert(usb_dev->descriptors.full_config);
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
	errno_t rc = usb_pipe_initialize_from_configuration(pipes, pipe_count,
	    usb_dev->descriptors.full_config,
	    usb_dev->descriptors.full_config_size,
	    usb_dev->bus_session);
	if (rc != EOK) {
		free(pipes);
		return rc;
	}

	/* Register created pipes. */
	unsigned pipes_registered = 0;
	for (size_t i = 0; i < pipe_count; i++) {
		if (pipes[i].present) {
			rc = usb_pipe_register(&pipes[i].pipe, pipes[i].descriptor, pipes[i].companion_descriptor);
			if (rc != EOK) {
				goto rollback_unregister_endpoints;
			}
		}
		pipes_registered++;
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
	for (size_t i = 0; i < pipes_registered; i++) {
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
 *
 */
void usb_device_destroy_pipes(usb_device_t *usb_dev)
{
	assert(usb_dev);
	assert(usb_dev->pipes || usb_dev->pipes_count == 0);

	/* Destroy the pipes. */
	int rc;
	for (size_t i = 0; i < usb_dev->pipes_count; ++i) {
		usb_log_debug2("Unregistering pipe %zu: %spresent.",
		    i, usb_dev->pipes[i].present ? "" : "not ");

		rc = usb_device_unmap_ep(usb_dev->pipes + i);
		if (rc != EOK && rc != ENOENT)
			usb_log_warning("Unregistering pipe %zu failed: %s", i, str_error(rc));
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

int usb_device_unmap_ep(usb_endpoint_mapping_t *epm)
{
	assert(epm);

	if (!epm->present)
		return ENOENT;

	const int rc = usb_pipe_unregister(&epm->pipe);
	if (rc != EOK)
		return rc;

	epm->present = false;
	return EOK;
}

usb_address_t usb_device_get_address(const usb_device_t *usb_dev)
{
	assert(usb_dev);
	return usb_dev->depth;
}

unsigned usb_device_get_depth(const usb_device_t *usb_dev)
{
	assert(usb_dev);
	return usb_dev->depth;
}

usb_speed_t usb_device_get_speed(const usb_device_t *usb_dev)
{
	assert(usb_dev);
	return usb_dev->speed;
}

int usb_device_get_iface_number(const usb_device_t *usb_dev)
{
	assert(usb_dev);
	return usb_dev->interface_no;
}

devman_handle_t usb_device_get_devman_handle(const usb_device_t *usb_dev)
{
	assert(usb_dev);
	return usb_dev->handle;
}

const usb_device_descriptors_t *usb_device_descriptors(usb_device_t *usb_dev)
{
	assert(usb_dev);
	return &usb_dev->descriptors;
}

const usb_alternate_interfaces_t * usb_device_get_alternative_ifaces(
    usb_device_t *usb_dev)
{
	assert(usb_dev);
	return &usb_dev->alternate_interfaces;
}

/** Clean instance of a USB device.
 *
 * @param dev Device to be de-initialized.
 *
 * Does not free/destroy supplied pointer.
 */
static void usb_device_fini(usb_device_t *usb_dev)
{
	if (usb_dev) {
		/* Destroy existing pipes. */
		usb_device_destroy_pipes(usb_dev);
		/* Ignore errors and hope for the best. */
		usb_alternate_interfaces_deinit(&usb_dev->alternate_interfaces);
		usb_device_release_descriptors(usb_dev);
		free(usb_dev->driver_data);
		usb_dev->driver_data = NULL;
		usb_dev_disconnect(usb_dev->bus_session);
		usb_dev->bus_session = NULL;
	}
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
static errno_t usb_device_init(usb_device_t *usb_dev, ddf_dev_t *ddf_dev,
    const usb_endpoint_description_t **endpoints, const char **errstr_ptr)
{
	assert(usb_dev != NULL);
	assert(errstr_ptr);

	*errstr_ptr = NULL;

	usb_dev->ddf_dev = ddf_dev;
	usb_dev->driver_data = NULL;
	usb_dev->descriptors.full_config = NULL;
	usb_dev->descriptors.full_config_size = 0;
	usb_dev->pipes_count = 0;
	usb_dev->pipes = NULL;

	usb_dev->bus_session = usb_dev_connect(usb_dev->handle);

	if (!usb_dev->bus_session) {
		*errstr_ptr = "device bus session create";
		return ENOMEM;
	}

	/* This pipe was registered by the hub driver,
	 * during device initialization. */
	errno_t rc = usb_pipe_initialize_default_control(&usb_dev->ctrl_pipe, usb_dev->bus_session);
	if (rc != EOK) {
		usb_dev_disconnect(usb_dev->bus_session);
		*errstr_ptr = "default control pipe initialization";
		return rc;
	}

	/* Retrieve standard descriptors. */
	rc = usb_device_retrieve_descriptors(usb_dev);
	if (rc != EOK) {
		*errstr_ptr = "descriptor retrieval";
		usb_dev_disconnect(usb_dev->bus_session);
		return rc;
	}

	/* Create alternate interfaces. We will silently ignore failure.
	 * We might either control one interface or an entire device,
	 * it makes no sense to speak about alternate interfaces when
	 * controlling a device. */
	usb_alternate_interfaces_init(&usb_dev->alternate_interfaces,
	    usb_dev->descriptors.full_config,
	    usb_dev->descriptors.full_config_size, usb_dev->interface_no);

	if (endpoints) {
		/* Create and register other pipes than default control (EP 0)*/
		rc = usb_device_create_pipes(usb_dev, endpoints);
		if (rc != EOK) {
			usb_device_fini(usb_dev);
			*errstr_ptr = "pipes initialization";
			return rc;
		}
	}

	return EOK;
}

static errno_t usb_device_get_info(async_sess_t *sess, usb_device_t *dev)
{
	assert(dev);

	async_exch_t *exch = async_exchange_begin(sess);
	if (!exch)
		return EPARTY;

	usb_device_desc_t dev_desc;
	const errno_t ret = usb_get_my_description(exch, &dev_desc);

	if (ret == EOK) {
		dev->address = dev_desc.address;
		dev->depth = dev_desc.depth;
		dev->speed = dev_desc.speed;
		dev->handle = dev_desc.handle;
		dev->interface_no = dev_desc.iface;
	}

	async_exchange_end(exch);
	return ret;
}

errno_t usb_device_create_ddf(ddf_dev_t *ddf_dev,
    const usb_endpoint_description_t **desc, const char **err)
{
	assert(ddf_dev);
	assert(err);

	async_sess_t *sess = ddf_dev_parent_sess_get(ddf_dev);
	if (sess == NULL)
		return ENOMEM;

	usb_device_t *usb_dev =
	    ddf_dev_data_alloc(ddf_dev, sizeof(usb_device_t));
	if (usb_dev == NULL) {
		*err = "DDF data alloc";
		return ENOMEM;
	}

	const errno_t ret = usb_device_get_info(sess, usb_dev);
	if (ret != EOK)
		return ret;

	return usb_device_init(usb_dev, ddf_dev, desc, err);
}

void usb_device_destroy_ddf(ddf_dev_t *ddf_dev)
{
	assert(ddf_dev);
	usb_device_t *usb_dev = ddf_dev_data_get(ddf_dev);
	assert(usb_dev);
	usb_device_fini(usb_dev);
	return;
}

usb_device_t * usb_device_create(devman_handle_t handle)
{
	usb_device_t *usb_dev = malloc(sizeof(usb_device_t));
	if (!usb_dev)
		return NULL;

	async_sess_t *sess = devman_device_connect(handle, IPC_FLAG_BLOCKING);
	errno_t ret = usb_device_get_info(sess, usb_dev);
	if (sess)
		async_hangup(sess);
	if (ret != EOK) {
		free(usb_dev);
		return NULL;
	}

	const char* dummy = NULL;
	ret = usb_device_init(usb_dev, NULL, NULL, &dummy);
	if (ret != EOK) {
		free(usb_dev);
		usb_dev = NULL;
	}
	return usb_dev;
}

void usb_device_destroy(usb_device_t *usb_dev)
{
	if (usb_dev) {
		usb_device_fini(usb_dev);
		free(usb_dev);
	}
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
