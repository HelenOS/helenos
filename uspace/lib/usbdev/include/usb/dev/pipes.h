/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup libusbdev
 * @{
 */
/** @file
 * USB pipes representation.
 */
#ifndef LIBUSBDEV_PIPES_H_
#define LIBUSBDEV_PIPES_H_

#include <usb/usb.h>
#include <usb/descriptor.h>
#include <usb_iface.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CTRL_PIPE_MIN_PACKET_SIZE 8

/** Abstraction of a logical connection to USB device endpoint.
 * It contains some vital information about the pipe.
 * This endpoint must be bound with existing usb_device_connection_t
 * (i.e. the wire to send data over).
 */
typedef struct {
	/** Pipe description received from HC */
	usb_pipe_desc_t desc;

	/** Whether to automatically reset halt on the endpoint.
	 * Valid only for control endpoint zero.
	 */
	bool auto_reset_halt;
	/** The connection used for sending the data. */
	usb_dev_session_t *bus_session;
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
	/** Relevant superspeed companion descriptor. */
	const usb_superspeed_endpoint_companion_descriptor_t
	    *companion_descriptor;
	/** Interface descriptor the endpoint belongs to. */
	const usb_standard_interface_descriptor_t *interface;
	/** Whether the endpoint was actually found. */
	bool present;
} usb_endpoint_mapping_t;

errno_t usb_pipe_initialize(usb_pipe_t *, usb_dev_session_t *);
errno_t usb_pipe_initialize_default_control(usb_pipe_t *, usb_dev_session_t *);

errno_t usb_pipe_initialize_from_configuration(usb_endpoint_mapping_t *,
    size_t, const uint8_t *, size_t, usb_dev_session_t *);

errno_t usb_pipe_register(usb_pipe_t *,
    const usb_standard_endpoint_descriptor_t *,
    const usb_superspeed_endpoint_companion_descriptor_t *);
errno_t usb_pipe_unregister(usb_pipe_t *);

errno_t usb_pipe_read(usb_pipe_t *, void *, size_t, size_t *);
errno_t usb_pipe_write(usb_pipe_t *, const void *, size_t);

errno_t usb_pipe_read_dma(usb_pipe_t *, void *, void *, size_t, size_t *);
errno_t usb_pipe_write_dma(usb_pipe_t *, void *, void *, size_t);

errno_t usb_pipe_control_read(usb_pipe_t *, const void *, size_t,
    void *, size_t, size_t *);
errno_t usb_pipe_control_write(usb_pipe_t *, const void *, size_t,
    const void *, size_t);

void *usb_pipe_alloc_buffer(usb_pipe_t *, size_t);
void usb_pipe_free_buffer(usb_pipe_t *, void *);
#endif
/**
 * @}
 */
