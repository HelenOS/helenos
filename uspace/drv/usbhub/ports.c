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
#include "usbhub_private.h"
#include "port_status.h"


/** Information for fibril for device discovery. */
struct add_device_phase1 {
	usb_hub_info_t *hub;
	size_t port;
	usb_speed_t speed;
};

static void usb_hub_removed_device(
	usb_hub_info_t * hub, uint16_t port);

static void usb_hub_port_reset_completed(usb_hub_info_t * hub,
	uint16_t port, uint32_t status);

static void usb_hub_port_over_current(usb_hub_info_t * hub,
	uint16_t port, uint32_t status);

static int get_port_status(usb_pipe_t *ctrl_pipe, size_t port,
    usb_port_status_t *status);

static int enable_port_callback(int port_no, void *arg);

static int add_device_phase1_worker_fibril(void *arg);

static int create_add_device_fibril(usb_hub_info_t *hub, size_t port,
    usb_speed_t speed);

/**
 * Process interrupts on given hub port
 *
 * Accepts connection, over current and port reset change.
 * @param hub hub representation
 * @param port port number, starting from 1
 */
void usb_hub_process_interrupt(usb_hub_info_t * hub,
	uint16_t port) {
	usb_log_debug("interrupt at port %zu\n", (size_t) port);
	//determine type of change
	//usb_pipe_t *pipe = hub->control_pipe;

	int opResult;

	usb_port_status_t status;
	opResult = get_port_status(&hub->usb_device->ctrl_pipe, port, &status);
	if (opResult != EOK) {
		usb_log_error("Failed to get port %zu status: %s.\n",
		    (size_t) port, str_error(opResult));
		return;
	}
	//connection change
	if (usb_port_is_status(status, USB_HUB_FEATURE_C_PORT_CONNECTION)) {
		bool device_connected = usb_port_is_status(status,
		    USB_HUB_FEATURE_PORT_CONNECTION);
		usb_log_debug("Connection change on port %zu: %s.\n",
		    (size_t) port,
		    device_connected ? "device attached" : "device removed");

		if (device_connected) {
			opResult = create_add_device_fibril(hub, port,
			    usb_port_speed(status));
			if (opResult != EOK) {
				usb_log_error(
				    "Cannot handle change on port %zu: %s.\n",
				    (size_t) port, str_error(opResult));
			}
		} else {
			usb_hub_removed_device(hub, port);
		}
	}
	//over current
	if (usb_port_is_status(status, USB_HUB_FEATURE_C_PORT_OVER_CURRENT)) {
		//check if it was not auto-resolved
		usb_log_debug("overcurrent change on port\n");
		usb_hub_port_over_current(hub, port, status);
	}
	//port reset
	if (usb_port_is_status(status, USB_HUB_FEATURE_C_PORT_RESET)) {
		usb_hub_port_reset_completed(hub, port, status);
	}
	usb_log_debug("status x%x : %d\n ", status, status);

	usb_port_status_set_bit(
	    &status, USB_HUB_FEATURE_C_PORT_CONNECTION,false);
	usb_port_status_set_bit(
	    &status, USB_HUB_FEATURE_PORT_RESET,false);
	usb_port_status_set_bit(
	    &status, USB_HUB_FEATURE_C_PORT_RESET,false);
	usb_port_status_set_bit(
	    &status, USB_HUB_FEATURE_C_PORT_OVER_CURRENT,false);
	/// \TODO what about port power change?
	unsigned int bit_idx;
	for(bit_idx = 16;bit_idx<32;++bit_idx){
		if(status & (1<<bit_idx)){
			usb_log_info(
			    "there was not yet handled change on port %d: %d"
			    ";clearing it\n",
			port, bit_idx);
			int opResult = usb_hub_clear_port_feature(
			    hub->control_pipe,
			    port, bit_idx);
			if (opResult != EOK) {
				usb_log_warning(
				    "could not clear port flag %d: %d\n",
				    bit_idx, opResult
				    );
			}
			usb_port_status_set_bit(
			    &status, bit_idx,false);
		}
	}
	if (status >> 16) {
		usb_log_info("there was a mistake on port %d "
		    "(not cleared status change): %X\n",
			port, status);
	}
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
static void usb_hub_removed_device(
	usb_hub_info_t * hub, uint16_t port) {

	int opResult = usb_hub_clear_port_feature(hub->control_pipe,
		port, USB_HUB_FEATURE_C_PORT_CONNECTION);
	if (opResult != EOK) {
		usb_log_warning("could not clear port-change-connection flag\n");
	}
	/** \TODO remove device from device manager - not yet implemented in
	 * devide manager
	 */

	//close address
	if(hub->ports[port].attached_device.address >= 0){
		/*uncomment this code to use it when DDF allows device removal
		opResult = usb_hc_unregister_device(
			&hub->connection,
			hub->attached_devs[port].address);
		if(opResult != EOK) {
			dprintf(USB_LOG_LEVEL_WARNING, "could not release "
				"address of "
			    "removed device: %d", opResult);
		}
		hub->attached_devs[port].address = 0;
		hub->attached_devs[port].handle = 0;
		 */
	} else {
		usb_log_warning("Device removed before being registered.\n");

		/*
		 * Device was removed before port reset completed.
		 * We will announce a failed port reset to unblock the
		 * port reset callback from new device wrapper.
		 */
		usb_hub_port_t *the_port = hub->ports + port;
		fibril_mutex_lock(&the_port->reset_mutex);
		the_port->reset_completed = true;
		the_port->reset_okay = false;
		fibril_condvar_broadcast(&the_port->reset_cv);
		fibril_mutex_unlock(&the_port->reset_mutex);
	}
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
static void usb_hub_port_reset_completed(usb_hub_info_t * hub,
	uint16_t port, uint32_t status){
	usb_log_debug("Port %zu reset complete.\n", (size_t) port);
	if (usb_port_is_status(status, USB_HUB_FEATURE_PORT_ENABLE)) {
		/* Finalize device adding. */
		usb_hub_port_t *the_port = hub->ports + port;
		fibril_mutex_lock(&the_port->reset_mutex);
		the_port->reset_completed = true;
		the_port->reset_okay = true;
		fibril_condvar_broadcast(&the_port->reset_cv);
		fibril_mutex_unlock(&the_port->reset_mutex);
	} else {
		usb_log_warning(
		    "Port %zu reset complete but port not enabled.\n",
		    (size_t) port);
	}
	/* Clear the port reset change. */
	int rc = usb_hub_clear_port_feature(hub->control_pipe,
	    port, USB_HUB_FEATURE_C_PORT_RESET);
	if (rc != EOK) {
		usb_log_error("Failed to clear port %d reset feature: %s.\n",
		    port, str_error(rc));
	}
}

/**
 * Process over current condition on port.
 *
 * Turn off the power on the port.
 *
 * @param hub hub representation
 * @param port port number, starting from 1
 */
static void usb_hub_port_over_current(usb_hub_info_t * hub,
	uint16_t port, uint32_t status) {
	int opResult;
	if(usb_port_is_status(status, USB_HUB_FEATURE_PORT_OVER_CURRENT)){
		opResult = usb_hub_clear_port_feature(hub->control_pipe,
			port, USB_HUB_FEATURE_PORT_POWER);
		if (opResult != EOK) {
			usb_log_error("cannot power off port %d;  %d\n",
				port, opResult);
		}
	}else{
		opResult = usb_hub_set_port_feature(hub->control_pipe,
			port, USB_HUB_FEATURE_PORT_POWER);
		if (opResult != EOK) {
			usb_log_error("cannot power on port %d;  %d\n",
				port, opResult);
		}
	}
}

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
	usb_device_request_setup_packet_t request;
	usb_port_status_t status_tmp;

	usb_hub_set_port_status_request(&request, port);
	int rc = usb_pipe_control_read(ctrl_pipe,
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
 * @param arg Custom argument, points to @c usb_hub_info_t.
 * @return Error code.
 */
static int enable_port_callback(int port_no, void *arg)
{
	usb_hub_info_t *hub = arg;
	int rc;
	usb_device_request_setup_packet_t request;
	usb_hub_port_t *my_port = hub->ports + port_no;

	usb_hub_set_reset_port_request(&request, port_no);
	rc = usb_pipe_control_write(hub->control_pipe,
	    &request, sizeof(request), NULL, 0);
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
	struct add_device_phase1 *data
	    = (struct add_device_phase1 *) arg;

	usb_address_t new_address;
	devman_handle_t child_handle;

	int rc = usb_hc_new_device_wrapper(data->hub->usb_device->ddf_dev,
	    &data->hub->connection, data->speed,
	    enable_port_callback, (int) data->port, data->hub,
	    &new_address, &child_handle,
	    NULL, NULL, NULL);

	if (rc != EOK) {
		usb_log_error("Failed registering device on port %zu: %s.\n",
		    data->port, str_error(rc));
		goto leave;
	}

	data->hub->ports[data->port].attached_device.handle = child_handle;
	data->hub->ports[data->port].attached_device.address = new_address;

	usb_log_info("Detected new device on `%s' (port %zu), "
	    "address %d (handle %" PRIun ").\n",
	    data->hub->usb_device->ddf_dev->name, data->port,
	    new_address, child_handle);

leave:
	free(arg);

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
	    = malloc(sizeof(struct add_device_phase1));
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

	int rc = usb_hub_clear_port_feature(hub->control_pipe, port,
	    USB_HUB_FEATURE_C_PORT_CONNECTION);
	if (rc != EOK) {
		free(data);
		usb_log_warning("Failed to clear port change flag: %s.\n",
		    str_error(rc));
		return rc;
	}

	fid_t fibril = fibril_create(add_device_phase1_worker_fibril, data);
	if (fibril == 0) {
		free(data);
		return ENOMEM;
	}
	fibril_add_ready(fibril);

	return EOK;
}

/**
 * @}
 */
