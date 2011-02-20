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
 * @brief USB driver (implementation).
 */
#include <usb/usbdrv.h>
#include <errno.h>


/** Find handle of host controller the device is physically attached to.
 *
 * @param[in] dev Device looking for its host controller.
 * @param[out] handle Host controller devman handle.
 * @return Error code.
 */
int usb_drv_find_hc(device_t *dev, devman_handle_t *handle)
{
	return ENOTSUP;
}

/** Connect to host controller the device is physically attached to.
 *
 * @param dev Device asking for connection.
 * @param hc_handle Devman handle of the host controller.
 * @param flags Connection flags (blocking connection).
 * @return Phone to the HC or error code.
 */
int usb_drv_hc_connect(device_t *dev, devman_handle_t hc_handle,
    unsigned int flags)
{
	return ENOTSUP;
}

/** Connect to host controller the device is physically attached to.
 *
 * @param dev Device asking for connection.
 * @param flags Connection flags (blocking connection).
 * @return Phone to corresponding HC or error code.
 */
int usb_drv_hc_connect_auto(device_t *dev, unsigned int flags)
{
	return ENOTSUP;
}

/** Tell USB address assigned to given device.
 *
 * @param phone Phone to my HC.
 * @param dev Device in question.
 * @return USB address or error code.
 */
usb_address_t usb_drv_get_my_address(int phone, device_t *dev)
{
	return ENOTSUP;
}

/** Tell HC to reserve default address.
 *
 * @param phone Open phone to host controller driver.
 * @return Error code.
 */
int usb_drv_reserve_default_address(int phone)
{
	return ENOTSUP;
}

/** Tell HC to release default address.
 *
 * @param phone Open phone to host controller driver.
 * @return Error code.
 */
int usb_drv_release_default_address(int phone)
{
	return ENOTSUP;
}

/** Ask HC for free address assignment.
 *
 * @param phone Open phone to host controller driver.
 * @return Assigned USB address or negative error code.
 */
usb_address_t usb_drv_request_address(int phone)
{
	return ENOTSUP;
}

/** Inform HC about binding address with devman handle.
 *
 * @param phone Open phone to host controller driver.
 * @param address Address to be binded.
 * @param handle Devman handle of the device.
 * @return Error code.
 */
int usb_drv_bind_address(int phone, usb_address_t address,
    devman_handle_t handle)
{
	return ENOTSUP;
}

/** Inform HC about address release.
 *
 * @param phone Open phone to host controller driver.
 * @param address Address to be released.
 * @return Error code.
 */
int usb_drv_release_address(int phone, usb_address_t address)
{
	return ENOTSUP;
}

/** Blocks caller until given USB transaction is finished.
 * After the transaction is finished, the user can access all output data
 * given to initial call function.
 *
 * @param handle Transaction handle.
 * @return Error status.
 * @retval EOK No error.
 * @retval EBADMEM Invalid handle.
 * @retval ENOENT Data buffer associated with transaction does not exist.
 */
int usb_drv_async_wait_for(usb_handle_t handle)
{
	return ENOTSUP;
}

/** Send interrupt data to device. */
int usb_drv_async_interrupt_out(int phone, usb_target_t target,
    void *buffer, size_t size,
    usb_handle_t *handle)
{
	return ENOTSUP;
}

/** Request interrupt data from device. */
int usb_drv_async_interrupt_in(int phone, usb_target_t target,
    void *buffer, size_t size, size_t *actual_size,
    usb_handle_t *handle)
{
	return ENOTSUP;
}

/** Start control write transfer. */
int usb_drv_async_control_write_setup(int phone, usb_target_t target,
    void *buffer, size_t size,
    usb_handle_t *handle)
{
	return ENOTSUP;
}

/** Send data during control write transfer. */
int usb_drv_async_control_write_data(int phone, usb_target_t target,
    void *buffer, size_t size,
    usb_handle_t *handle)
{
	return ENOTSUP;
}

/** Finalize control write transfer. */
int usb_drv_async_control_write_status(int phone, usb_target_t target,
    usb_handle_t *handle)
{
	return ENOTSUP;
}

/** Issue whole control write transfer. */
int usb_drv_async_control_write(int phone, usb_target_t target,
    void *setup_packet, size_t setup_packet_size,
    void *buffer, size_t buffer_size,
    usb_handle_t *handle)
{
	return ENOTSUP;
}

/** Start control read transfer. */
int usb_drv_async_control_read_setup(int phone, usb_target_t target,
    void *buffer, size_t size,
    usb_handle_t *handle)
{
	return ENOTSUP;
}

/** Read data during control read transfer. */
int usb_drv_async_control_read_data(int phone, usb_target_t target,
    void *buffer, size_t size, size_t *actual_size,
    usb_handle_t *handle)
{
	return ENOTSUP;
}

/** Finalize control read transfer. */
int usb_drv_async_control_read_status(int phone, usb_target_t target,
    usb_handle_t *handle)
{
	return ENOTSUP;
}

/** Issue whole control read transfer. */
int usb_drv_async_control_read(int phone, usb_target_t target,
    void *setup_packet, size_t setup_packet_size,
    void *buffer, size_t buffer_size, size_t *actual_size,
    usb_handle_t *handle)
{
	return ENOTSUP;
}

/**
 * @}
 */
