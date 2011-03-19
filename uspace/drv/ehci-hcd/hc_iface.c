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
/** @addtogroup drvusbehci
 * @{
 */
/** @file
 * USB-HC interface implementation.
 */
#include <ddf/driver.h>
#include <ddf/interrupt.h>
#include <device/hw_res.h>
#include <errno.h>
#include <str_error.h>

#include <usb_iface.h>
#include <usb/ddfiface.h>
#include <usb/debug.h>

#include "ehci.h"

#define UNSUPPORTED(methodname) \
	usb_log_warning("Unsupported interface method `%s()' in %s:%d.\n", \
	    methodname, __FILE__, __LINE__)


static int reserve_default_address(ddf_fun_t *fun, usb_speed_t speed)
{
	UNSUPPORTED("reserve_default_address");

	return ENOTSUP;
}

static int release_default_address(ddf_fun_t *fun)
{
	UNSUPPORTED("release_default_address");

	return ENOTSUP;
}

static int request_address(ddf_fun_t *fun, usb_speed_t speed, usb_address_t *address)
{
	UNSUPPORTED("request_address");

	return ENOTSUP;
}

static int bind_address(ddf_fun_t *fun, usb_address_t address, devman_handle_t handle)
{
	UNSUPPORTED("bind_address");

	return ENOTSUP;
}

static int release_address(ddf_fun_t *fun, usb_address_t address)
{
	UNSUPPORTED("release_address");

	return ENOTSUP;
}

static int register_endpoint(ddf_fun_t *fun, usb_address_t address, usb_endpoint_t endpoint,
    usb_transfer_type_t transfer_type, usb_direction_t direction,
    size_t max_packet_size, unsigned int interval)
{
	UNSUPPORTED("register_endpoint");

	return ENOTSUP;
}

static int unregister_endpoint(ddf_fun_t *fun, usb_address_t address, usb_endpoint_t endpoint,
    usb_direction_t direction)
{
	UNSUPPORTED("unregister_endpoint");

	return ENOTSUP;
}

static int interrupt_out(ddf_fun_t *fun, usb_target_t target,
    size_t max_packet_size, void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	UNSUPPORTED("interrupt_out");

	return ENOTSUP;
}

static int interrupt_in(ddf_fun_t *fun, usb_target_t target,
    size_t max_packet_size, void *data, size_t size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	UNSUPPORTED("interrupt_in");

	return ENOTSUP;
}

static int bulk_out(ddf_fun_t *fun, usb_target_t target,
    size_t max_packet_size, void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	UNSUPPORTED("bulk_out");

	return ENOTSUP;
}

static int bulk_in(ddf_fun_t *fun, usb_target_t target,
    size_t max_packet_size, void *data, size_t size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	UNSUPPORTED("bulk_in");

	return ENOTSUP;
}

static int control_write(ddf_fun_t *fun, usb_target_t target,
    size_t max_packet_size,
    void *setup_packet, size_t setup_packet_size, void *data_buffer, size_t data_buffer_size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	UNSUPPORTED("control_write");

	return ENOTSUP;
}

static int control_read(ddf_fun_t *fun, usb_target_t target,
    size_t max_packet_size,
    void *setup_packet, size_t setup_packet_size, void *data_buffer, size_t data_buffer_size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	UNSUPPORTED("control_read");

	return ENOTSUP;
}


usbhc_iface_t ehci_hc_iface = {
	.reserve_default_address = reserve_default_address,
	.release_default_address = release_default_address,
	.request_address = request_address,
	.bind_address = bind_address,
	.release_address = release_address,

	.register_endpoint = register_endpoint,
	.unregister_endpoint = unregister_endpoint,

	.interrupt_out = interrupt_out,
	.interrupt_in = interrupt_in,

	.bulk_out = bulk_out,
	.bulk_in = bulk_in,

	.control_write = control_write,
	.control_read = control_read
};

/**
 * @}
 */
