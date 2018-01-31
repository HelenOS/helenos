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

/** @addtogroup drvusbvhc
 * @{
 */
/** @file
 * @brief Representation of an USB hub.
 */
#ifndef VHC_HUB_HUB_H_
#define VHC_HUB_HUB_H_

#include <fibril_synch.h>

#ifndef HUB_PORT_COUNT
#define HUB_PORT_COUNT 2
#endif
#define BITS2BYTES(bits) (bits ? ((((bits)-1)>>3)+1) : 0)

/** Hub port internal state.
 * Some states (e.g. port over current) are not covered as they are not
 * simulated at all.
 */
typedef enum {
	HUB_PORT_STATE_UNKNOWN,
	HUB_PORT_STATE_NOT_CONFIGURED,
	HUB_PORT_STATE_POWERED_OFF,
	HUB_PORT_STATE_DISCONNECTED,
	HUB_PORT_STATE_DISABLED,
	HUB_PORT_STATE_RESETTING,
	HUB_PORT_STATE_ENABLED,
	HUB_PORT_STATE_SUSPENDED,
	HUB_PORT_STATE_RESUMING,
	/* HUB_PORT_STATE_, */
} hub_port_state_t;

char hub_port_state_to_char(hub_port_state_t);

/** Hub status change mask bits. */
typedef enum {
	HUB_STATUS_C_PORT_CONNECTION = (1 << 0),
	HUB_STATUS_C_PORT_ENABLE = (1 << 1),
	HUB_STATUS_C_PORT_SUSPEND = (1 << 2),
	HUB_STATUS_C_PORT_OVER_CURRENT = (1 << 3),
	HUB_STATUS_C_PORT_RESET = (1 << 4),
	/* HUB_STATUS_C_ = (1 << ), */
} hub_status_change_t;

typedef struct hub hub_t;

/** Hub port information. */
typedef struct {
	/** Custom pointer to connected device. */
	void *connected_device;
	/** Port index (one based). */
	size_t index;
	/** Port state. */
	hub_port_state_t state;
	/** Status change bitmap. */
	uint16_t status_change;
	/** Containing hub. */
	hub_t *hub;
} hub_port_t;

/** Hub device type. */
struct hub {
	/** Hub ports. */
	hub_port_t ports[HUB_PORT_COUNT];
	/** Custom hub data. */
	void *custom_data;
	/** Access guard to the whole hub. */
	fibril_mutex_t guard;
	/** Last value of status change bitmap. */
	bool signal_changes;
};

void hub_init(hub_t *);
size_t hub_connect_device(hub_t *, void *);
errno_t hub_disconnect_device(hub_t *, void *);
size_t hub_find_device(hub_t *, void *);
void hub_acquire(hub_t *);
void hub_release(hub_t *);
void hub_set_port_state(hub_t *, size_t, hub_port_state_t);
void hub_set_port_state_all(hub_t *, hub_port_state_t);
hub_port_state_t hub_get_port_state(hub_t *, size_t);
void hub_clear_port_status_change(hub_t *, size_t, hub_status_change_t);
uint16_t hub_get_port_status_change(hub_t *, size_t);
uint32_t hub_get_port_status(hub_t *, size_t);
uint8_t hub_get_status_change_bitmap(hub_t *);


#endif
/**
 * @}
 */
