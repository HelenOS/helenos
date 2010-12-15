/*
 * Copyright (c) 2010 Vojtech Horky
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

/** @addtogroup usb
 * @{
 */
/** @file
 * @brief Virtual USB hub.
 */
#include <usb/classes/classes.h>
#include <usbvirt/hub.h>
#include <usbvirt/device.h>
#include <errno.h>
#include <str_error.h>
#include <stdlib.h>
#include <driver.h>
#include <usb/usbdrv.h>

#include "hub.h"


/** Produce a byte from bit values.
 */
#define MAKE_BYTE(b0, b1, b2, b3, b4, b5, b6, b7) \
	(( \
		((b0) << 0) \
		| ((b1) << 1) \
		| ((b2) << 2) \
		| ((b3) << 3) \
		| ((b4) << 4) \
		| ((b5) << 5) \
		| ((b6) << 6) \
		| ((b7) << 7) \
	))

/* Static functions. */
static hub_port_t *get_hub_port(hub_t *, size_t);
static void set_port_status_change(hub_port_t *, hub_status_change_t);
static void clear_port_status_change(hub_port_t *, uint16_t);
static int set_port_state_delayed_fibril(void *);
static void set_port_state_delayed(hub_t *, size_t, suseconds_t,
    hub_port_state_t, hub_port_state_t);

/** Convert hub port state to a char. */
char hub_port_state_to_char(hub_port_state_t state) {
	switch (state) {
		case HUB_PORT_STATE_NOT_CONFIGURED:
			return '-';
		case HUB_PORT_STATE_POWERED_OFF:
			return 'O';
		case HUB_PORT_STATE_DISCONNECTED:
			return 'X';
		case HUB_PORT_STATE_DISABLED:
			return 'D';
		case HUB_PORT_STATE_RESETTING:
			return 'R';
		case HUB_PORT_STATE_ENABLED:
			return 'E';
		case HUB_PORT_STATE_SUSPENDED:
			return 'S';
		case HUB_PORT_STATE_RESUMING:
			return 'F';
		default:
			return '?';
	}
}

/** Initialize single hub port.
 *
 * @param port Port to be initialized.
 * @param index Port index (one based).
 */
static void hub_init_port(hub_port_t *port, size_t index)
{
	port->connected_device = NULL;
	port->index = index;
	port->state = HUB_PORT_STATE_NOT_CONFIGURED;
	port->status_change = 0;
}

/** Initialize the hub.
 *
 * @param hub Hub to be initialized.
 */
void hub_init(hub_t *hub)
{
	size_t i;
	for (i = 0; i < HUB_PORT_COUNT; i++) {
		hub_init_port(&hub->ports[i], i + 1);
	}
	hub->custom_data = NULL;
	fibril_mutex_initialize(&hub->guard);
}

/** Connect a device to the hub.
 *
 * @param hub Hub to connect device to.
 * @param device Device to be connected.
 * @return Index of port the device was connected to.
 * @retval -1 No free port available.
 */
size_t hub_connect_device(hub_t *hub, void *device)
{
	size_t i;
	for (i = 0; i < HUB_PORT_COUNT; i++) {
		hub_port_t *port = &hub->ports[i];

		if (port->connected_device != NULL) {
			continue;
		}

		port->connected_device = device;

		/*
		 * TODO:
		 * If the hub was configured, we can normally
		 * announce the plug-in.
		 * Otherwise, we will wait until hub is configured
		 * and announce changes in single burst.
		 */
		//if (port->state == HUB_PORT_STATE_DISCONNECTED) {
			port->state = HUB_PORT_STATE_DISABLED;
			set_port_status_change(port, HUB_STATUS_C_PORT_CONNECTION);
		//}

		return i;
	}

	return (size_t) -1;
}

/** Find port device is connected to.
 *
 * @param hub Hub in question.
 * @param device Device in question.
 * @return Port index (zero based).
 * @retval -1 Device is not connected.
 */
size_t hub_find_device(hub_t *hub, void *device)
{
	size_t i;
	for (i = 0; i < HUB_PORT_COUNT; i++) {
		hub_port_t *port = &hub->ports[i];

		if (port->connected_device == device) {
			return i;
		}
	}

	return 0;
}

/** Acquire exclusive access to the hub.
 *
 * @param hub Hub in question.
 */
void hub_acquire(hub_t *hub)
{
	fibril_mutex_lock(&hub->guard);
}

/** Give up exclusive access to the hub.
 *
 * @param hub Hub in question.
 */
void hub_release(hub_t *hub)
{
	fibril_mutex_unlock(&hub->guard);
}

/** Change port state.
 *
 * @param hub Hub the port belongs to.
 * @param port_index Port index (zero based).
 * @param state New port state.
 */
void hub_set_port_state(hub_t *hub, size_t port_index, hub_port_state_t state)
{
	hub_port_t *port = get_hub_port(hub, port_index);
	if (port == NULL) {
		return;
	}

	switch (state) {
		case HUB_PORT_STATE_POWERED_OFF:
			clear_port_status_change(port, HUB_STATUS_C_PORT_CONNECTION);
			clear_port_status_change(port, HUB_STATUS_C_PORT_ENABLE);
			clear_port_status_change(port, HUB_STATUS_C_PORT_RESET);
			break;
		case HUB_PORT_STATE_RESUMING:
			set_port_state_delayed(hub, port_index,
			    10, state, HUB_PORT_STATE_ENABLED);
			break;
		case HUB_PORT_STATE_RESETTING:
			set_port_state_delayed(hub, port_index,
			    10, state, HUB_PORT_STATE_ENABLED);
			break;
		case HUB_PORT_STATE_ENABLED:
			if (port->state == HUB_PORT_STATE_RESETTING) {
				set_port_status_change(port, HUB_STATUS_C_PORT_RESET);
			}
			break;
		default:
			break;
	}

	port->state = state;
}

/** Change state of all ports.
 *
 * @param hub Hub in question.
 * @param state New state for all ports.
 */
void hub_set_port_state_all(hub_t *hub, hub_port_state_t state)
{
	size_t i;
	for (i = 0; i < HUB_PORT_COUNT; i++) {
		hub_set_port_state(hub, i, state);
	}
}

/** Get port state.
 *
 * @param hub Hub the port belongs to.
 * @param port_index Port index (zero based).
 * @return Port state.
 */
hub_port_state_t hub_get_port_state(hub_t *hub, size_t port_index)
{
	hub_port_t *port = get_hub_port(hub, port_index);
	if (port == NULL) {
		return HUB_PORT_STATE_UNKNOWN;
	}

	return port->state;
}

/** Clear port status change bit.
 *
 * @param hub Hub the port belongs to.
 * @param port_index Port index (zero based).
 * @param change Change to be cleared.
 */
void hub_clear_port_status_change(hub_t *hub, size_t port_index,
    hub_status_change_t change)
{
	hub_port_t *port = get_hub_port(hub, port_index);
	if (port == NULL) {
		return;
	}

	clear_port_status_change(port, change);
}

/** Get port status change bits.
 *
 * @param hub Hub the port belongs to.
 * @param port_index Port index (zero based).
 * @return Port status change bitmap in standard format.
 */
uint16_t hub_get_port_status_change(hub_t *hub, size_t port_index)
{
	hub_port_t *port = get_hub_port(hub, port_index);
	if (port == NULL) {
		return 0;
	}

	return port->status_change;
}

/** Get port status bits.
 *
 * @param hub Hub the port belongs to.
 * @param port_index Port index (zero based).
 * @return Port status bitmap in standard format.
 */
uint32_t hub_get_port_status(hub_t *hub, size_t port_index)
{
	hub_port_t *port = get_hub_port(hub, port_index);
	if (port == NULL) {
		return 0;
	}

	uint32_t status;
	status = MAKE_BYTE(
	    /* Current connect status. */
	    port->connected_device == NULL ? 0 : 1,
	    /* Port enabled/disabled. */
	    port->state == HUB_PORT_STATE_ENABLED ? 1 : 0,
	    /* Suspend. */
	    (port->state == HUB_PORT_STATE_SUSPENDED)
		|| (port->state == HUB_PORT_STATE_RESUMING) ? 1 : 0,
	    /* Over-current. */
	    0,
	    /* Reset. */
	    port->state == HUB_PORT_STATE_RESETTING ? 1 : 0,
	    /* Reserved. */
	    0, 0, 0)

	    | (MAKE_BYTE(
	    /* Port power. */
	    port->state == HUB_PORT_STATE_POWERED_OFF ? 0 : 1,
	    /* Full-speed device. */
	    0,
	    /* Reserved. */
	    0, 0, 0, 0, 0, 0
	    )) << 8;

	status |= (port->status_change << 16);

	return status;
}

uint8_t hub_get_status_change_bitmap(hub_t *hub)
{
	uint8_t change_map = 0;

	size_t i;
	for (i = 0; i < HUB_PORT_COUNT; i++) {
		hub_port_t *port = &hub->ports[i];

		if (port->status_change != 0) {
			change_map |= (1 << port->index);
		}
	}

	return change_map;
}


/*
 *
 * STATIC (HELPER) FUNCTIONS
 *
 */

static hub_port_t *get_hub_port(hub_t *hub, size_t port)
{
	if (port >= HUB_PORT_COUNT) {
		return NULL;
	}

	return &hub->ports[port];
}

static void set_port_status_change(hub_port_t *port,
    hub_status_change_t change)
{
	assert(port != NULL);
	port->status_change |= change;
}

static void clear_port_status_change(hub_port_t *port,
    uint16_t change)
{
	assert(port != NULL);
	port->status_change &= (~change);
}

struct delay_port_state_change {
	suseconds_t delay;
	hub_port_state_t old_state;
	hub_port_state_t new_state;
	size_t port;
	hub_t *hub;
};

static int set_port_state_delayed_fibril(void *arg)
{
	struct delay_port_state_change *change
	    = (struct delay_port_state_change *) arg;

	async_usleep(change->delay);

	hub_acquire(change->hub);

	hub_port_t *port = get_hub_port(change->hub, change->port);
	assert(port != NULL);

	if (port->state == change->old_state) {
		hub_set_port_state(change->hub, change->port,
		    change->new_state);
	}

	hub_release(change->hub);

	free(change);

	return EOK;
}

static void set_port_state_delayed(hub_t *hub, size_t port_index,
    suseconds_t delay_time_ms,
    hub_port_state_t old_state, hub_port_state_t new_state)
{
	struct delay_port_state_change *change
	    = malloc(sizeof(struct delay_port_state_change));

	change->hub = hub;
	change->port = port_index;
	change->delay = delay_time_ms * 1000;
	change->old_state = old_state;
	change->new_state = new_state;
	fid_t fibril = fibril_create(set_port_state_delayed_fibril, change);
	if (fibril == 0) {
		printf("Failed to create fibril\n");
		free(change);
		return;
	}
	fibril_add_ready(fibril);
}



/**
 * @}
 */
