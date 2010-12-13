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

/** @addtogroup libusb usb
 * @{
 */
/** @file
 * @brief Implementation of pseudo-synchronous transfers.
 */
#include <usb/usbdrv.h>
#include <usbhc_iface.h>
#include <errno.h>

int usb_drv_psync_interrupt_out(int phone, usb_target_t target,
    void *buffer, size_t size)
{
	usb_handle_t h;
	int rc;
	rc = usb_drv_async_interrupt_out(phone, target, buffer, size, &h);
	if (rc != EOK) {
		return rc;
	}
	return usb_drv_async_wait_for(h);
}

int usb_drv_psync_interrupt_in(int phone, usb_target_t target,
    void *buffer, size_t size, size_t *actual_size)
{
	usb_handle_t h;
	int rc;
	rc = usb_drv_async_interrupt_in(phone, target, buffer, size,
	    actual_size, &h);
	if (rc != EOK) {
		return rc;
	}
	return usb_drv_async_wait_for(h);
}



int usb_drv_psync_control_write_setup(int phone, usb_target_t target,
    void *buffer, size_t size)
{
	usb_handle_t h;
	int rc;
	rc = usb_drv_async_control_write_setup(phone, target, buffer, size, &h);
	if (rc != EOK) {
		return rc;
	}
	return usb_drv_async_wait_for(h);
}

int usb_drv_psync_control_write_data(int phone, usb_target_t target,
    void *buffer, size_t size)
{
	usb_handle_t h;
	int rc;
	rc = usb_drv_async_control_write_data(phone, target, buffer, size, &h);
	if (rc != EOK) {
		return rc;
	}
	return usb_drv_async_wait_for(h);
}

int usb_drv_psync_control_write_status(int phone, usb_target_t target)
{
	usb_handle_t h;
	int rc;
	rc = usb_drv_async_control_write_status(phone, target, &h);
	if (rc != EOK) {
		return rc;
	}
	return usb_drv_async_wait_for(h);
}


/** Perform complete control write transaction over USB.
 *
 * The DATA stage is performed only when @p data is not NULL and
 * @p data_size is greater than zero.
 *
 * @param phone Open phone to host controller.
 * @param target Target device and endpoint.
 * @param setup_packet Setup packet data.
 * @param setup_packet_size Size of the setup packet.
 * @param data Data to be sent.
 * @param data_size Size of the @p data buffer.
 * @return Error code.
 */
int usb_drv_psync_control_write(int phone, usb_target_t target,
    void *setup_packet, size_t setup_packet_size,
    void *data, size_t data_size)
{
	int rc;
	
	rc = usb_drv_psync_control_write_setup(phone, target,
	    setup_packet, setup_packet_size);
	if (rc != EOK) {
		return rc;
	}

	if ((data != NULL) && (data_size > 0)) {
		rc = usb_drv_psync_control_write_data(phone, target,
		    data, data_size);
		if (rc != EOK) {
			return rc;
		}
	}

	rc = usb_drv_psync_control_write_status(phone, target);

	return rc;
}


int usb_drv_psync_control_read_setup(int phone, usb_target_t target,
    void *buffer, size_t size)
{
	usb_handle_t h;
	int rc;
	rc = usb_drv_async_control_read_setup(phone, target, buffer, size, &h);
	if (rc != EOK) {
		return rc;
	}
	return usb_drv_async_wait_for(h);
}

int usb_drv_psync_control_read_data(int phone, usb_target_t target,
    void *buffer, size_t size, size_t *actual_size)
{
	usb_handle_t h;
	int rc;
	rc = usb_drv_async_control_read_data(phone, target, buffer, size,
	    actual_size, &h);
	if (rc != EOK) {
		return rc;
	}
	return usb_drv_async_wait_for(h);
}

int usb_drv_psync_control_read_status(int phone, usb_target_t target)
{
	usb_handle_t h;
	int rc;
	rc = usb_drv_async_control_read_status(phone, target, &h);
	if (rc != EOK) {
		return rc;
	}
	return usb_drv_async_wait_for(h);
}


/** Perform complete control read transaction over USB.
 *
 * @param phone Open phone to host controller.
 * @param target Target device and endpoint.
 * @param setup_packet Setup packet data.
 * @param setup_packet_size Size of the setup packet.
 * @param data Storage for read data.
 * @param data_size Size of the @p data buffer.
 * @param actual_data_size Storage for number of actually transferred data from
 *        device.
 * @return Error code.
 */
int usb_drv_psync_control_read(int phone, usb_target_t target,
    void *setup_packet, size_t setup_packet_size,
    void *data, size_t data_size, size_t *actual_data_size)
{
	int rc;
	
	rc = usb_drv_psync_control_read_setup(phone, target,
	    setup_packet, setup_packet_size);
	if (rc != EOK) {
		return rc;
	}

	rc = usb_drv_psync_control_read_data(phone, target,
	    data, data_size, actual_data_size);
	if (rc != EOK) {
		return rc;
	}

	rc = usb_drv_psync_control_read_status(phone, target);

	return rc;
}




/**
 * @}
 */
