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
/** @addtogroup libusbdev
 * @{
 */
/** @file
 * USB pipes representation.
 */
#ifndef LIBUSBDEV_PIPES_H_
#define LIBUSBDEV_PIPES_H_

#include <sys/types.h>
#include <ipc/devman.h>
#include <ddf/driver.h>
#include <fibril_synch.h>
#include <usb/usb.h>
#include <usb/descriptor.h>
#include <usb/dev/usb_device_connection.h>

#define CTRL_PIPE_MIN_PACKET_SIZE 8
/** Abstraction of a logical connection to USB device endpoint.
 * It encapsulates endpoint attributes (transfer type etc.).
 * This endpoint must be bound with existing usb_device_connection_t
 * (i.e. the wire to send data over).
 */
typedef struct {
	/** The connection used for sending the data. */
	usb_device_connection_t *wire;

	/** Endpoint number. */
	usb_endpoint_t endpoint_no;

	/** Endpoint transfer type. */
	usb_transfer_type_t transfer_type;

	/** Endpoint direction. */
	usb_direction_t direction;

	/** Maximum packet size for the endpoint. */
	size_t max_packet_size;

	/** Whether to automatically reset halt on the endpoint.
	 * Valid only for control endpoint zero.
	 */
	bool auto_reset_halt;
} usb_pipe_t;

/** Description of endpoint characteristics. */
typedef struct {
	/** Transfer type (e.g. control or interrupt). */
	usb_transfer_type_t transfer_type;
	/** Transfer direction (to or from a device). */
	usb_direction_t direction;
	/** Interface class this endpoint belongs to (-1 for any). */
	int interface_class;
	/** Interface subclass this endpoint belongs to (-1 for any). */
	int interface_subclass;
	/** Interface protocol this endpoint belongs to (-1 for any). */
	int interface_protocol;
	/** Extra endpoint flags. */
	unsigned int flags;
} usb_endpoint_description_t;

/** Mapping of endpoint pipes and endpoint descriptions. */
typedef struct {
	/** Endpoint pipe. */
	usb_pipe_t pipe;
	/** Endpoint description. */
	const usb_endpoint_description_t *description;
	/** Interface number the endpoint must belong to (-1 for any). */
	int interface_no;
	/** Alternate interface setting to choose. */
	int interface_setting;
	/** Found descriptor fitting the description. */
	const usb_standard_endpoint_descriptor_t *descriptor;
	/** Interface descriptor the endpoint belongs to. */
	const usb_standard_interface_descriptor_t *interface;
	/** Whether the endpoint was actually found. */
	bool present;
} usb_endpoint_mapping_t;

int usb_pipe_initialize(usb_pipe_t *, usb_device_connection_t *,
    usb_endpoint_t, usb_transfer_type_t, size_t, usb_direction_t);
int usb_pipe_initialize_default_control(usb_pipe_t *,
    usb_device_connection_t *);

int usb_pipe_probe_default_control(usb_pipe_t *);
int usb_pipe_initialize_from_configuration(usb_endpoint_mapping_t *,
    size_t, const uint8_t *, size_t, usb_device_connection_t *);

int usb_pipe_register(usb_pipe_t *, unsigned);
int usb_pipe_unregister(usb_pipe_t *);

int usb_pipe_start_long_transfer(usb_pipe_t *);
int usb_pipe_end_long_transfer(usb_pipe_t *);

int usb_pipe_read(usb_pipe_t *, void *, size_t, size_t *);
int usb_pipe_write(usb_pipe_t *, const void *, size_t);

int usb_pipe_control_read(usb_pipe_t *, const void *, size_t,
    void *, size_t, size_t *);
int usb_pipe_control_write(usb_pipe_t *, const void *, size_t,
    const void *, size_t);

#endif
/**
 * @}
 */
