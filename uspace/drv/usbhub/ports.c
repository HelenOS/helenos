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
#include "port_status.h"
#include <inttypes.h>
#include <errno.h>
#include <str_error.h>
#include <usb/request.h>
#include <usb/debug.h>

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

struct add_device_phase1 {
	usb_hub_info_t *hub;
	size_t port;
	usb_speed_t speed;
};

static int enable_port_callback(int port_no, void *arg)
{
	usb_hub_info_t *hub = (usb_hub_info_t *) arg;
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

	/* Clear the port reset change. */
	rc = usb_hub_clear_port_feature(hub->control_pipe,
	    port_no, USB_HUB_FEATURE_C_PORT_RESET);
	if (rc != EOK) {
		usb_log_error("Failed to clear port %d reset feature: %s.\n",
		    port_no, str_error(rc));
		return rc;
	}

	return EOK;
}

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

	usb_log_info("Detected new device on `%s' (port %zu), " \
	    "address %d (handle %" PRIun ").\n",
	    data->hub->usb_device->ddf_dev->name, data->port,
	    new_address, child_handle);

leave:
	free(arg);

	return EOK;
}

static int add_device_phase1_new_fibril(usb_hub_info_t *hub, size_t port,
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

static void process_port_change(usb_hub_info_t *hub, size_t port)
{
	int rc;

	usb_port_status_t port_status;

	rc = get_port_status(&hub->usb_device->ctrl_pipe, port, &port_status);
	if (rc != EOK) {
		usb_log_error("Failed to get port %zu status: %s.\n",
		    port, str_error(rc));
		return;
	}

	/*
	 * Check exact nature of the change.
	 */
	usb_log_debug("Port %zu change status: %x.\n", port,
	    (unsigned int) port_status);

	if (usb_port_connect_change(&port_status)) {
		bool device_connected = usb_port_dev_connected(&port_status);
		usb_log_debug("Connection change on port %zu: %s.\n", port,
		    device_connected ? "device attached" : "device removed");

		if (device_connected) {
			rc = add_device_phase1_new_fibril(hub, port,
			    usb_port_speed(&port_status));
			if (rc != EOK) {
				usb_log_error(
				    "Cannot handle change on port %zu: %s.\n",
				    str_error(rc));
			}
		} else {
			usb_hub_removed_device(hub, port);
		}
	}

	if (usb_port_overcurrent_change(&port_status)) {
		if (usb_port_over_current(&port_status)) {
			usb_log_warning("Overcurrent on port %zu.\n", port);
			usb_hub_over_current(hub, port);
		} else {
			usb_log_debug("Overcurrent on port %zu autoresolved.\n",
			    port);
		}
	}

	if (usb_port_reset_completed(&port_status)) {
		usb_log_debug("Port %zu reset complete.\n", port);
		if (usb_port_enabled(&port_status)) {
			/* Finalize device adding. */
			usb_hub_port_t *the_port = hub->ports + port;
			fibril_mutex_lock(&the_port->reset_mutex);
			the_port->reset_completed = true;
			fibril_condvar_broadcast(&the_port->reset_cv);
			fibril_mutex_unlock(&the_port->reset_mutex);
		} else {
			usb_log_warning(
			    "Port %zu reset complete but port not enabled.\n",
			    port);
		}
	}

	usb_port_set_connect_change(&port_status, false);
	usb_port_set_reset(&port_status, false);
	usb_port_set_reset_completed(&port_status, false);
	usb_port_set_dev_connected(&port_status, false);
	if (port_status >> 16) {
		usb_log_warning("Unsupported change on port %zu: %x.\n",
		    port, (unsigned int) port_status);
	}
}

bool hub_port_changes_callback(usb_device_t *dev,
    uint8_t *change_bitmap, size_t change_bitmap_size, void *arg)
{
	usb_hub_info_t *hub = (usb_hub_info_t *) arg;

	/* FIXME: check that we received enough bytes. */
	if (change_bitmap_size == 0) {
		goto leave;
	}

	size_t port;
	for (port = 1; port < hub->port_count + 1; port++) {
		bool change = (change_bitmap[port / 8] >> (port % 8)) % 2;
		if (change) {
			process_port_change(hub, port);
		}
	}


leave:
	/* FIXME: proper interval. */
	async_usleep(1000 * 1000 * 10 );

	return true;
}


/**
 * @}
 */
