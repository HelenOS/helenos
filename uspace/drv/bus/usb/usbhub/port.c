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
/** @addtogroup drvusbhub
 * @{
 */
/** @file
 * Hub ports functions.
 */

#include <stdbool.h>
#include <errno.h>
#include <str_error.h>
#include <inttypes.h>
#include <fibril_synch.h>

#include <usb/debug.h>
#include <usb/dev/hub.h>

#include "port.h"
#include "usbhub.h"
#include "status.h"

/** Information for fibril for device discovery. */
struct add_device_phase1 {
	usb_hub_dev_t *hub;
	usb_hub_port_t *port;
	usb_speed_t speed;
};

static int usb_hub_port_device_gone(usb_hub_port_t *port, usb_hub_dev_t *hub);
static void usb_hub_port_reset_completed(usb_hub_port_t *port,
    usb_port_status_t status);
static int get_port_status(usb_hub_port_t *port, usb_port_status_t *status);
static int enable_port_callback(void *arg);
static int add_device_phase1_worker_fibril(void *arg);
static int create_add_device_fibril(usb_hub_port_t *port, usb_hub_dev_t *hub,
    usb_speed_t speed);

int usb_hub_port_fini(usb_hub_port_t *port, usb_hub_dev_t *hub)
{
	assert(port);
	if (port->attached_device.fun)
		return usb_hub_port_device_gone(port, hub);
	return EOK;
}

/**
 * Clear feature on hub port.
 *
 * @param port Port structure.
 * @param feature Feature selector.
 * @return Operation result
 */
int usb_hub_port_clear_feature(
    usb_hub_port_t *port, usb_hub_class_feature_t feature)
{
	assert(port);
	const usb_device_request_setup_packet_t clear_request = {
		.request_type = USB_HUB_REQ_TYPE_CLEAR_PORT_FEATURE,
		.request = USB_DEVREQ_CLEAR_FEATURE,
		.value = feature,
		.index = port->port_number,
		.length = 0,
	};
	return usb_pipe_control_write(port->control_pipe, &clear_request,
	    sizeof(clear_request), NULL, 0);
}

/**
 * Set feature on hub port.
 *
 * @param port Port structure.
 * @param feature Feature selector.
 * @return Operation result
 */
int usb_hub_port_set_feature(
    usb_hub_port_t *port, usb_hub_class_feature_t feature)
{
	assert(port);
	const usb_device_request_setup_packet_t clear_request = {
		.request_type = USB_HUB_REQ_TYPE_SET_PORT_FEATURE,
		.request = USB_DEVREQ_SET_FEATURE,
		.index = port->port_number,
		.value = feature,
		.length = 0,
	};
	return usb_pipe_control_write(port->control_pipe, &clear_request,
	    sizeof(clear_request), NULL, 0);
}

/**
 * Mark reset process as failed due to external reasons
 *
 * @param port Port structure
 */
void usb_hub_port_reset_fail(usb_hub_port_t *port)
{
	assert(port);
	fibril_mutex_lock(&port->mutex);
	port->reset_completed = true;
	port->reset_okay = false;
	fibril_condvar_broadcast(&port->reset_cv);
	fibril_mutex_unlock(&port->mutex);
}

/**
 * Process interrupts on given port
 *
 * Accepts connection, over current and port reset change.
 * @param port port structure
 * @param hub hub representation
 */
void usb_hub_port_process_interrupt(usb_hub_port_t *port, usb_hub_dev_t *hub)
{
	assert(port);
	assert(hub);
	usb_log_debug("Interrupt at port %zu\n", port->port_number);

	usb_port_status_t status = 0;
	const int opResult = get_port_status(port, &status);
	if (opResult != EOK) {
		usb_log_error("Failed to get port %zu status: %s.\n",
		    port->port_number, str_error(opResult));
		return;
	}

	/* Connection change */
	if (status & USB_HUB_PORT_C_STATUS_CONNECTION) {
		const bool connected =
		    (status & USB_HUB_PORT_STATUS_CONNECTION) != 0;
		usb_log_debug("Connection change on port %zu: device %s.\n",
		    port->port_number, connected ? "attached" : "removed");

		/* ACK the change */
		const int opResult = usb_hub_port_clear_feature(port,
		    USB_HUB_FEATURE_C_PORT_CONNECTION);
		if (opResult != EOK) {
			usb_log_warning("Failed to clear port-change-connection"
			    " flag: %s.\n", str_error(opResult));
		}

		if (connected) {
			const int opResult = create_add_device_fibril(port, hub,
			    usb_port_speed(status));
			if (opResult != EOK) {
				usb_log_error(
				    "Cannot handle change on port %zu: %s.\n",
				    port->port_number, str_error(opResult));
			}
		} else {
			/* If enabled change was reported leave the removal
			 * to that handler, it shall ACK the change too. */
			if (!(status & USB_HUB_PORT_C_STATUS_ENABLED)) {
				usb_hub_port_device_gone(port, hub);
			}
		}
	}

	/* Enable change, ports are automatically disabled on errors. */
	if (status & USB_HUB_PORT_C_STATUS_ENABLED) {
		usb_log_info("Port %zu, disabled because of errors.\n",
		   port->port_number);
		usb_hub_port_device_gone(port, hub);
		const int rc = usb_hub_port_clear_feature(port,
		        USB_HUB_FEATURE_C_PORT_ENABLE);
		if (rc != EOK) {
			usb_log_error(
			    "Failed to clear port %zu enable change feature: "
			    "%s.\n", port->port_number, str_error(rc));
		}

	}

	/* Suspend change */
	if (status & USB_HUB_PORT_C_STATUS_SUSPEND) {
		usb_log_error("Port %zu went to suspend state, this should"
		    "NOT happen as we do not support suspend state!",
		    port->port_number);
		const int rc = usb_hub_port_clear_feature(port,
		        USB_HUB_FEATURE_C_PORT_SUSPEND);
		if (rc != EOK) {
			usb_log_error(
			    "Failed to clear port %zu suspend change feature: "
			    "%s.\n", port->port_number, str_error(rc));
		}
	}

	/* Over current */
	if (status & USB_HUB_PORT_C_STATUS_OC) {
		/* According to the USB specs:
		 * 11.13.5 Over-current Reporting and Recovery
		 * Hub device is responsible for putting port in power off
		 * mode. USB system software is responsible for powering port
		 * back on when the over-current condition is gone */
		const int rc = usb_hub_port_clear_feature(port,
		    USB_HUB_FEATURE_C_PORT_OVER_CURRENT);
		if (rc != EOK) {
			usb_log_error(
			    "Failed to clear port %zu OC change feature: %s.\n",
			    port->port_number, str_error(rc));
		}
		if (!(status & ~USB_HUB_PORT_STATUS_OC)) {
			const int rc = usb_hub_port_set_feature(
			    port, USB_HUB_FEATURE_PORT_POWER);
			if (rc != EOK) {
				usb_log_error(
				    "Failed to set port %zu power after OC:"
				    " %s.\n", port->port_number, str_error(rc));
			}
		}
	}

	/* Port reset, set on port reset complete. */
	if (status & USB_HUB_PORT_C_STATUS_RESET) {
		usb_hub_port_reset_completed(port, status);
	}

	usb_log_debug("Port %zu status 0x%08" PRIx32 "\n",
	    port->port_number, status);
}

/**
 * routine called when a device on port has been removed
 *
 * If the device on port had default address, it releases default address.
 * Otherwise does not do anything, because DDF does not allow to remove device
 * from it`s device tree.
 * @param port port structure
 * @param hub hub representation
 */
int usb_hub_port_device_gone(usb_hub_port_t *port, usb_hub_dev_t *hub)
{
	assert(port);
	assert(hub);
	if (port->attached_device.address < 0) {
		usb_log_warning(
		    "Device on port %zu removed before being registered.\n",
		    port->port_number);

		/*
		 * Device was removed before port reset completed.
		 * We will announce a failed port reset to unblock the
		 * port reset callback from new device wrapper.
		 */
		usb_hub_port_reset_fail(port);
		return EOK;
	}

	fibril_mutex_lock(&port->mutex);
	assert(port->attached_device.fun);
	usb_log_debug("Removing device on port %zu.\n", port->port_number);
	int ret = ddf_fun_unbind(port->attached_device.fun);
	if (ret != EOK) {
		usb_log_error("Failed to unbind child function on port"
		    " %zu: %s.\n", port->port_number, str_error(ret));
		fibril_mutex_unlock(&port->mutex);
		return ret;
	}

	ddf_fun_destroy(port->attached_device.fun);
	port->attached_device.fun = NULL;

	ret = usb_hub_unregister_device(&hub->usb_device->hc_conn,
	    &port->attached_device);
	if (ret != EOK) {
		usb_log_warning("Failed to unregister address of the "
		    "removed device: %s.\n", str_error(ret));
	}

	port->attached_device.address = -1;
	fibril_mutex_unlock(&port->mutex);
	usb_log_info("Removed device on port %zu.\n", port->port_number);
	return EOK;
}

/**
 * Process port reset change
 *
 * After this change port should be enabled, unless some problem occurred.
 * This functions triggers second phase of enabling new device.
 * @param port Port structure
 * @param status Port status mask
 */
void usb_hub_port_reset_completed(usb_hub_port_t *port,
    usb_port_status_t status)
{
	assert(port);
	fibril_mutex_lock(&port->mutex);
	/* Finalize device adding. */
	port->reset_completed = true;
	port->reset_okay = (status & USB_HUB_PORT_STATUS_ENABLED) != 0;

	if (port->reset_okay) {
		usb_log_debug("Port %zu reset complete.\n", port->port_number);
	} else {
		usb_log_warning(
		    "Port %zu reset complete but port not enabled.\n",
		    port->port_number);
	}
	fibril_condvar_broadcast(&port->reset_cv);
	fibril_mutex_unlock(&port->mutex);

	/* Clear the port reset change. */
	int rc = usb_hub_port_clear_feature(port, USB_HUB_FEATURE_C_PORT_RESET);
	if (rc != EOK) {
		usb_log_error(
		    "Failed to clear port %zu reset change feature: %s.\n",
		    port->port_number, str_error(rc));
	}
}

/** Retrieve port status.
 *
 * @param[in] port Port structure
 * @param[out] status Where to store the port status.
 * @return Error code.
 */
static int get_port_status(usb_hub_port_t *port, usb_port_status_t *status)
{
	assert(port);
	/* USB hub specific GET_PORT_STATUS request. See USB Spec 11.16.2.6
	 * Generic GET_STATUS request cannot be used because of the difference
	 * in status data size (2B vs. 4B)*/
	const usb_device_request_setup_packet_t request = {
		.request_type = USB_HUB_REQ_TYPE_GET_PORT_STATUS,
		.request = USB_HUB_REQUEST_GET_STATUS,
		.value = 0,
		.index = uint16_host2usb(port->port_number),
		.length = sizeof(usb_port_status_t),
	};
	size_t recv_size;
	usb_port_status_t status_tmp;

	const int rc = usb_pipe_control_read(port->control_pipe,
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

/** Callback for enabling a specific port.
 *
 * We wait on a CV until port is reseted.
 * That is announced via change on interrupt pipe.
 *
 * @param port_no Port number (starting at 1).
 * @param arg Custom argument, points to @c usb_hub_dev_t.
 * @return Error code.
 */
static int enable_port_callback(void *arg)
{
	usb_hub_port_t *port = arg;
	assert(port);
	const int rc =
	    usb_hub_port_set_feature(port, USB_HUB_FEATURE_PORT_RESET);
	if (rc != EOK) {
		usb_log_warning("Port reset failed: %s.\n", str_error(rc));
		return rc;
	}

	/*
	 * Wait until reset completes.
	 */
	fibril_mutex_lock(&port->mutex);
	while (!port->reset_completed) {
		fibril_condvar_wait(&port->reset_cv, &port->mutex);
	}
	fibril_mutex_unlock(&port->mutex);

	return port->reset_okay ? EOK : ESTALL;
}

/** Fibril for adding a new device.
 *
 * Separate fibril is needed because the port reset completion is announced
 * via interrupt pipe and thus we cannot block here.
 *
 * @param arg Pointer to struct add_device_phase1.
 * @return 0 Always.
 */
int add_device_phase1_worker_fibril(void *arg)
{
	struct add_device_phase1 *data = arg;
	assert(data);

	usb_address_t new_address;
	ddf_fun_t *child_fun;

	child_fun = ddf_fun_create(data->hub->usb_device->ddf_dev,
	    fun_inner, NULL);
	if (child_fun == NULL)
		return ENOMEM;

	const int rc = usb_hc_new_device_wrapper(data->hub->usb_device->ddf_dev,
	    child_fun, &data->hub->usb_device->hc_conn, data->speed,
	    enable_port_callback, data->port, &new_address, NULL);

	if (rc == EOK) {
		fibril_mutex_lock(&data->port->mutex);
		data->port->attached_device.fun = child_fun;
		data->port->attached_device.address = new_address;
		fibril_mutex_unlock(&data->port->mutex);

		usb_log_info("Detected new device on `%s' (port %zu), "
		    "address %d (handle %" PRIun ").\n",
		    ddf_dev_get_name(data->hub->usb_device->ddf_dev),
		    data->port->port_number, new_address,
		    ddf_fun_get_handle(child_fun));
	} else {
		ddf_fun_destroy(child_fun);
		usb_log_error("Failed registering device on port %zu: %s.\n",
		    data->port->port_number, str_error(rc));
	}


	fibril_mutex_lock(&data->hub->pending_ops_mutex);
	assert(data->hub->pending_ops_count > 0);
	--data->hub->pending_ops_count;
	fibril_condvar_signal(&data->hub->pending_ops_cv);
	fibril_mutex_unlock(&data->hub->pending_ops_mutex);

	free(arg);

	return rc;
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
static int create_add_device_fibril(usb_hub_port_t *port, usb_hub_dev_t *hub,
    usb_speed_t speed)
{
	assert(hub);
	assert(port);
	struct add_device_phase1 *data
	    = malloc(sizeof(struct add_device_phase1));
	if (data == NULL) {
		return ENOMEM;
	}
	data->hub = hub;
	data->port = port;
	data->speed = speed;

	fibril_mutex_lock(&port->mutex);
	port->reset_completed = false;
	fibril_mutex_unlock(&port->mutex);

	fid_t fibril = fibril_create(add_device_phase1_worker_fibril, data);
	if (fibril == 0) {
		free(data);
		return ENOMEM;
	}
	fibril_mutex_lock(&hub->pending_ops_mutex);
	++hub->pending_ops_count;
	fibril_mutex_unlock(&hub->pending_ops_mutex);
	fibril_add_ready(fibril);

	return EOK;
}

/**
 * @}
 */
