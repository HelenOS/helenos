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

/** @addtogroup drvusbhub
 * @{
 */
/** @file
 * Hub ports functions.
 */

#include <bool.h>
#include <errno.h>
#include <str_error.h>
#include <inttypes.h>
#include <fibril_synch.h>

#include <usb/debug.h>

#include "ports.h"
#include "usbhub.h"
#include "utils.h"
#include "port_status.h"

/** Information for fibril for device discovery. */
struct add_device_phase1 {
	usb_hub_info_t *hub;
	size_t port;
	usb_speed_t speed;
};

static void usb_hub_removed_device(usb_hub_info_t *hub, size_t port);
static void usb_hub_port_reset_completed(const usb_hub_info_t *hub,
    size_t port, usb_port_status_t status);
static int get_port_status(usb_pipe_t *ctrl_pipe, size_t port,
    usb_port_status_t *status);
static int enable_port_callback(int port_no, void *arg);
static int add_device_phase1_worker_fibril(void *arg);
static int create_add_device_fibril(usb_hub_info_t *hub, size_t port,
    usb_speed_t speed);

/**
 * Clear feature on hub port.
 *
 * @param hc Host controller telephone
 * @param address Hub address
 * @param port_index Port
 * @param feature Feature selector
 * @return Operation result
 */
int usb_hub_clear_port_feature(usb_pipe_t *pipe,
    int port_index, usb_hub_class_feature_t feature)
{
	usb_device_request_setup_packet_t clear_request = {
		.request_type = USB_HUB_REQ_TYPE_CLEAR_PORT_FEATURE,
		.request = USB_DEVREQ_CLEAR_FEATURE,
		.value = feature,
		.index = port_index,
		.length = 0,
	};
	return usb_pipe_control_write(pipe, &clear_request,
	    sizeof(clear_request), NULL, 0);
}
/*----------------------------------------------------------------------------*/
/**
 * Clear feature on hub port.
 *
 * @param hc Host controller telephone
 * @param address Hub address
 * @param port_index Port
 * @param feature Feature selector
 * @return Operation result
 */
int usb_hub_set_port_feature(usb_pipe_t *pipe,
    int port_index, usb_hub_class_feature_t feature)
{

	usb_device_request_setup_packet_t clear_request = {
		.request_type = USB_HUB_REQ_TYPE_SET_PORT_FEATURE,
		.request = USB_DEVREQ_SET_FEATURE,
		.index = port_index,
		.value = feature,
		.length = 0,
	};
	return usb_pipe_control_write(pipe, &clear_request,
	    sizeof(clear_request), NULL, 0);
}

/**
 * Process interrupts on given hub port
 *
 * Accepts connection, over current and port reset change.
 * @param hub hub representation
 * @param port port number, starting from 1
 */
void usb_hub_process_port_interrupt(usb_hub_info_t *hub, size_t port)
{
	usb_log_debug("Interrupt at port %zu\n", port);

	usb_port_status_t status;
	const int opResult =
	    get_port_status(&hub->usb_device->ctrl_pipe, port, &status);
	if (opResult != EOK) {
		usb_log_error("Failed to get port %zu status: %s.\n",
		    port, str_error(opResult));
		return;
	}

	/* Connection change */
	if (status & USB_HUB_PORT_C_STATUS_CONNECTION) {
		const bool device_connected =
		    (status & USB_HUB_PORT_STATUS_CONNECTION) != 0;
		usb_log_debug("Connection change on port %zu: device %s.\n",
		    port, device_connected ? "attached" : "removed");
		/* ACK the change */
		const int opResult =
		    usb_hub_clear_port_feature(hub->control_pipe,
		    port, USB_HUB_FEATURE_C_PORT_CONNECTION);
		if (opResult != EOK) {
			usb_log_warning("Failed to clear "
			    "port-change-connection flag: %s.\n",
			    str_error(opResult));
		}

		if (device_connected) {
			const int opResult = create_add_device_fibril(hub, port,
			    usb_port_speed(status));
			if (opResult != EOK) {
				usb_log_error(
				    "Cannot handle change on port %zu: %s.\n",
				    port, str_error(opResult));
			}
		} else {
			usb_hub_removed_device(hub, port);
		}
	}

	/* Enable change, ports are automatically disabled on errors. */
	if (status & USB_HUB_PORT_C_STATUS_ENABLED) {
		// TODO: Remove device that was connected
		// TODO: Clear feature C_PORT_ENABLE

	}

	/* Suspend change */
	if (status & USB_HUB_PORT_C_STATUS_SUSPEND) {
		usb_log_error("Port %zu went to suspend state, this should"
		    "NOT happen as we do not support suspend state!", port);
		// TODO: Clear feature C_PORT_SUSPEND
	}

	/* Over current */
	if (status & USB_HUB_PORT_C_STATUS_OC) {
		/* According to the USB specs:
		 * 11.13.5 Over-current Reporting and Recovery
		 * Hub device is responsible for putting port in power off
		 * mode. USB system software is responsible for powering port
		 * back on when the over-curent condition is gone */
		if (!(status & ~USB_HUB_PORT_STATUS_OC)) {
			// TODO: Power port on, this will cause connect
			// change and device initialization.
		}
		// TODO: Ack over-power change.
	}

	/* Port reset, set on port reset complete. */
	if (status & USB_HUB_PORT_C_STATUS_RESET) {
		usb_hub_port_reset_completed(hub, port, status);
	}

	usb_log_debug("Port %zu status 0x%08" PRIx32 "\n", port, status);
}

/**
 * routine called when a device on port has been removed
 *
 * If the device on port had default address, it releases default address.
 * Otherwise does not do anything, because DDF does not allow to remove device
 * from it`s device tree.
 * @param hub hub representation
 * @param port port number, starting from 1
 */
static void usb_hub_removed_device(usb_hub_info_t *hub, size_t port)
{
	/** \TODO remove device from device manager - not yet implemented in
	 * devide manager
	 */
	usb_hub_port_t *the_port = hub->ports + port;

	fibril_mutex_lock(&hub->port_mutex);

	if (the_port->attached_device.address >= 0) {
		usb_log_warning("Device unplug on `%s' (port %zu): " \
		    "not implemented.\n", hub->usb_device->ddf_dev->name,
		    (size_t) port);
		the_port->attached_device.address = -1;
		the_port->attached_device.handle = 0;
	} else {
		usb_log_warning("Device removed before being registered.\n");

		/*
		 * Device was removed before port reset completed.
		 * We will announce a failed port reset to unblock the
		 * port reset callback from new device wrapper.
		 */
		fibril_mutex_lock(&the_port->reset_mutex);
		the_port->reset_completed = true;
		the_port->reset_okay = false;
		fibril_condvar_broadcast(&the_port->reset_cv);
		fibril_mutex_unlock(&the_port->reset_mutex);
	}

	fibril_mutex_unlock(&hub->port_mutex);
}

/**
 * Process port reset change
 *
 * After this change port should be enabled, unless some problem occured.
 * This functions triggers second phase of enabling new device.
 * @param hub
 * @param port
 * @param status
 */
static void usb_hub_port_reset_completed(const usb_hub_info_t *hub,
    size_t port, usb_port_status_t status)
{
	usb_hub_port_t *the_port = hub->ports + port;
	fibril_mutex_lock(&the_port->reset_mutex);
	/* Finalize device adding. */
	the_port->reset_completed = true;
	the_port->reset_okay = (status & USB_HUB_PORT_STATUS_ENABLED) != 0;

	if (the_port->reset_okay) {
		usb_log_debug("Port %zu reset complete.\n", port);
	} else {
		usb_log_warning(
		    "Port %zu reset complete but port not enabled.\n", port);
	}
	fibril_condvar_broadcast(&the_port->reset_cv);
	fibril_mutex_unlock(&the_port->reset_mutex);

	/* Clear the port reset change. */
	int rc = usb_hub_clear_port_feature(hub->control_pipe,
	    port, USB_HUB_FEATURE_C_PORT_RESET);
	if (rc != EOK) {
		usb_log_error("Failed to clear port %d reset feature: %s.\n",
		    port, str_error(rc));
	}
}
/*----------------------------------------------------------------------------*/
/** Retrieve port status.
 *
 * @param[in] ctrl_pipe Control pipe to use.
 * @param[in] port Port number (starting at 1).
 * @param[out] status Where to store the port status.
 * @return Error code.
 */
static int get_port_status(usb_pipe_t *ctrl_pipe, size_t port,
    usb_port_status_t *status)
{
	size_t recv_size;
	usb_port_status_t status_tmp;
	/* USB hub specific GET_PORT_STATUS request. See USB Spec 11.16.2.6
	 * Generic GET_STATUS request cannot be used because of the difference
	 * in status data size (2B vs. 4B)*/
	const usb_device_request_setup_packet_t request = {
		.request_type = USB_HUB_REQ_TYPE_GET_PORT_STATUS,
		.request = USB_HUB_REQUEST_GET_STATUS,
		.value = 0,
		.index = port,
		.length = sizeof(usb_port_status_t),
	};

	const int rc = usb_pipe_control_read(ctrl_pipe,
	    &request, sizeof(usb_device_request_setup_packet_t),
	    &status_tmp, sizeof(status_tmp), &recv_size);
	if (rc != EOK) {
		return rc;
	}

	if (recv_size != sizeof (status_tmp)) {
		return ELIMIT;
	}

	if (status != NULL) {
		*status = status_tmp;
	}

	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Callback for enabling a specific port.
 *
 * We wait on a CV until port is reseted.
 * That is announced via change on interrupt pipe.
 *
 * @param port_no Port number (starting at 1).
 * @param arg Custom argument, points to @c usb_hub_info_t.
 * @return Error code.
 */
static int enable_port_callback(int port_no, void *arg)
{
	usb_hub_info_t *hub = arg;
	assert(hub);
	usb_hub_port_t *my_port = hub->ports + port_no;
	const int rc = usb_hub_set_port_feature(hub->control_pipe,
		port_no, USB_HUB_FEATURE_PORT_RESET);
	if (rc != EOK) {
		usb_log_warning("Port reset failed: %s.\n", str_error(rc));
		return rc;
	}

	/*
	 * Wait until reset completes.
	 */
	fibril_mutex_lock(&my_port->reset_mutex);
	while (!my_port->reset_completed) {
		fibril_condvar_wait(&my_port->reset_cv, &my_port->reset_mutex);
	}
	fibril_mutex_unlock(&my_port->reset_mutex);

	if (my_port->reset_okay) {
		return EOK;
	} else {
		return ESTALL;
	}
}

/** Fibril for adding a new device.
 *
 * Separate fibril is needed because the port reset completion is announced
 * via interrupt pipe and thus we cannot block here.
 *
 * @param arg Pointer to struct add_device_phase1.
 * @return 0 Always.
 */
static int add_device_phase1_worker_fibril(void *arg)
{
	struct add_device_phase1 *data = arg;
	assert(data);

	usb_address_t new_address;
	devman_handle_t child_handle;

	const int rc = usb_hc_new_device_wrapper(data->hub->usb_device->ddf_dev,
	    &data->hub->connection, data->speed,
	    enable_port_callback, (int) data->port, data->hub,
	    &new_address, &child_handle,
	    NULL, NULL, NULL);

	if (rc != EOK) {
		usb_log_error("Failed registering device on port %zu: %s.\n",
		    data->port, str_error(rc));
		goto leave;
	}

	fibril_mutex_lock(&data->hub->port_mutex);
	data->hub->ports[data->port].attached_device.handle = child_handle;
	data->hub->ports[data->port].attached_device.address = new_address;
	fibril_mutex_unlock(&data->hub->port_mutex);

	usb_log_info("Detected new device on `%s' (port %zu), "
	    "address %d (handle %" PRIun ").\n",
	    data->hub->usb_device->ddf_dev->name, data->port,
	    new_address, child_handle);

leave:
	free(arg);

	fibril_mutex_lock(&data->hub->pending_ops_mutex);
	assert(data->hub->pending_ops_count > 0);
	data->hub->pending_ops_count--;
	fibril_condvar_signal(&data->hub->pending_ops_cv);
	fibril_mutex_unlock(&data->hub->pending_ops_mutex);


	return EOK;
}

/** Start device adding when connection change is detected.
 *
 * This fires a new fibril to complete the device addition.
 *
 * @param hub Hub where the change occured.
 * @param port Port index (starting at 1).
 * @param speed Speed of the device.
 * @return Error code.
 */
static int create_add_device_fibril(usb_hub_info_t *hub, size_t port,
    usb_speed_t speed)
{
	struct add_device_phase1 *data
	    = malloc(sizeof (struct add_device_phase1));
	if (data == NULL) {
		return ENOMEM;
	}
	data->hub = hub;
	data->port = port;
	data->speed = speed;

	usb_hub_port_t *the_port = hub->ports + port;

	fibril_mutex_lock(&the_port->reset_mutex);
	the_port->reset_completed = false;
	fibril_mutex_unlock(&the_port->reset_mutex);

	fid_t fibril = fibril_create(add_device_phase1_worker_fibril, data);
	if (fibril == 0) {
		free(data);
		return ENOMEM;
	}
	fibril_mutex_lock(&hub->pending_ops_mutex);
	hub->pending_ops_count++;
	fibril_mutex_unlock(&hub->pending_ops_mutex);
	fibril_add_ready(fibril);

	return EOK;
}

/**
 * @}
 */
