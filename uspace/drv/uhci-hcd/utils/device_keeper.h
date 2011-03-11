/*
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

/** @addtogroup drvusbuhci
 * @{
 */
/** @file
 * @brief UHCI driver
 */
#ifndef UTILS_DEVICE_KEEPER_H
#define UTILS_DEVICE_KEEPER_H
#include <devman.h>
#include <fibril_synch.h>
#include <usb/usb.h>

#define USB_ADDRESS_COUNT (USB11_ADDRESS_MAX + 1)

struct usb_device_info {
	usb_speed_t speed;
	bool occupied;
	uint16_t toggle_status;
	devman_handle_t handle;
};

typedef struct device_keeper {
	struct usb_device_info devices[USB_ADDRESS_COUNT];
	fibril_mutex_t guard;
	fibril_condvar_t default_address_occupied;
	usb_address_t last_address;
} device_keeper_t;

void device_keeper_init(device_keeper_t *instance);

void device_keeper_reserve_default(
    device_keeper_t *instance, usb_speed_t speed);

void device_keeper_release_default(device_keeper_t *instance);

void device_keeper_reset_if_need(
    device_keeper_t *instance, usb_target_t target, const unsigned char *setup_data);

int device_keeper_get_toggle(device_keeper_t *instance, usb_target_t target);

int device_keeper_set_toggle(
    device_keeper_t *instance, usb_target_t target, bool toggle);

usb_address_t device_keeper_request(
    device_keeper_t *instance, usb_speed_t speed);

void device_keeper_bind(
    device_keeper_t *instance, usb_address_t address, devman_handle_t handle);

void device_keeper_release(device_keeper_t *instance, usb_address_t address);

usb_address_t device_keeper_find(
    device_keeper_t *instance, devman_handle_t handle);

usb_speed_t device_keeper_speed(
    device_keeper_t *instance, usb_address_t address);
#endif
/**
 * @}
 */
