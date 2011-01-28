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

/** @addtogroup libusb
 * @{
 */
/** @file
 * @brief USB driver.
 */
#ifndef LIBUSB_USBDRV_H_
#define LIBUSB_USBDRV_H_

#include <usb/usb.h>
#include <driver.h>
#include <usb/devreq.h>
#include <usb/descriptor.h>

int usb_drv_find_hc(device_t *, devman_handle_t *);
int usb_drv_hc_connect(device_t *, devman_handle_t, unsigned int);
int usb_drv_hc_connect_auto(device_t *, unsigned int);

int usb_drv_reserve_default_address(int);
int usb_drv_release_default_address(int);
usb_address_t usb_drv_request_address(int);
int usb_drv_bind_address(int, usb_address_t, devman_handle_t);
int usb_drv_release_address(int, usb_address_t);

usb_address_t usb_drv_get_my_address(int, device_t *);

int usb_drv_async_interrupt_out(int, usb_target_t,
    void *, size_t, usb_handle_t *);
int usb_drv_async_interrupt_in(int, usb_target_t,
    void *, size_t, size_t *, usb_handle_t *);

int usb_drv_psync_interrupt_out(int, usb_target_t, void *, size_t);
int usb_drv_psync_interrupt_in(int, usb_target_t, void *, size_t, size_t *);



int usb_drv_async_control_write_setup(int, usb_target_t,
    void *, size_t, usb_handle_t *);
int usb_drv_async_control_write_data(int, usb_target_t,
    void *, size_t, usb_handle_t *);
int usb_drv_async_control_write_status(int, usb_target_t,
    usb_handle_t *);

int usb_drv_psync_control_write_setup(int, usb_target_t, void *, size_t);
int usb_drv_psync_control_write_data(int, usb_target_t, void *, size_t);
int usb_drv_psync_control_write_status(int, usb_target_t);

int usb_drv_psync_control_write(int, usb_target_t,
    void *, size_t, void *, size_t);


int usb_drv_async_control_read_setup(int, usb_target_t,
    void *, size_t, usb_handle_t *);
int usb_drv_async_control_read_data(int, usb_target_t,
    void *, size_t, size_t *, usb_handle_t *);
int usb_drv_async_control_read_status(int, usb_target_t,
    usb_handle_t *);

int usb_drv_psync_control_read_setup(int, usb_target_t, void *, size_t);
int usb_drv_psync_control_read_data(int, usb_target_t, void *, size_t, size_t *);
int usb_drv_psync_control_read_status(int, usb_target_t);

int usb_drv_psync_control_read(int, usb_target_t,
    void *, size_t, void *, size_t, size_t *);



int usb_drv_async_wait_for(usb_handle_t);

int usb_drv_create_match_ids_from_device_descriptor(match_id_list_t *,
    const usb_standard_device_descriptor_t *);
int usb_drv_create_match_ids_from_configuration_descriptor(match_id_list_t *,
    const void *, size_t);

int usb_drv_create_device_match_ids(int, match_id_list_t *, usb_address_t);
int usb_drv_register_child_in_devman(int, device_t *, usb_address_t,
    devman_handle_t *);


#endif
/**
 * @}
 */
