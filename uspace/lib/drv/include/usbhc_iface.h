/*
 * Copyright (c) 2010 Vojtech Horky
 * Copyright (c) 2017 Ondrej Hlavaty
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

/** @addtogroup libdrv
 * @addtogroup usb
 * @{
 */
/** @file
 * @brief USB host controler interface definition. This is the interface of
 * USB host controller function, which can be used by usb device drivers.
 */

#ifndef LIBDRV_USBHC_IFACE_H_
#define LIBDRV_USBHC_IFACE_H_

#include "ddf/driver.h"
#include <async.h>

/** USB speeds. */
typedef enum {
	/** USB 1.1 low speed (1.5Mbits/s). */
	USB_SPEED_LOW,
	/** USB 1.1 full speed (12Mbits/s). */
	USB_SPEED_FULL,
	/** USB 2.0 high speed (480Mbits/s). */
	USB_SPEED_HIGH,
	/** USB 3.0 super speed (5Gbits/s). */
	USB_SPEED_SUPER,
	/** Psuedo-speed serving as a boundary. */
	USB_SPEED_MAX
} usb_speed_t;

/** USB endpoint number type.
 * Negative values could be used to indicate error.
 */
typedef uint16_t usb_endpoint_t;

/** USB address type.
 * Negative values could be used to indicate error.
 */
typedef uint16_t usb_address_t;

/**
 * USB Stream ID type.
 */
typedef uint16_t usb_stream_t;

/** USB transfer type. */
typedef enum {
	USB_TRANSFER_CONTROL = 0,
	USB_TRANSFER_ISOCHRONOUS = 1,
	USB_TRANSFER_BULK = 2,
	USB_TRANSFER_INTERRUPT = 3,
} usb_transfer_type_t;

#define USB_TRANSFER_COUNT  (USB_TRANSFER_INTERRUPT + 1)

/** USB data transfer direction. */
typedef enum {
	USB_DIRECTION_IN,
	USB_DIRECTION_OUT,
	USB_DIRECTION_BOTH,
} usb_direction_t;

#define USB_DIRECTION_COUNT  (USB_DIRECTION_BOTH + 1)

/** USB complete address type.
 * Pair address + endpoint is identification of transaction recipient.
 */
typedef union {
	struct {
		usb_address_t address;
		usb_endpoint_t endpoint;
		usb_stream_t stream;
	} __attribute__((packed));
	uint64_t packed;
} usb_target_t;

// FIXME: DMA buffers shall be part of libdrv anyway.
typedef uintptr_t dma_policy_t;

typedef struct dma_buffer {
	void *virt;
	dma_policy_t policy;
} dma_buffer_t;

typedef struct usb_pipe_desc {
	/** Endpoint number. */
	usb_endpoint_t endpoint_no;

	/** Endpoint transfer type. */
	usb_transfer_type_t transfer_type;

	/** Endpoint direction. */
	usb_direction_t direction;

	/**
	 * Maximum size of one transfer. Non-periodic endpoints may handle
	 * bigger transfers, but those can be split into multiple USB transfers.
	 */
	size_t max_transfer_size;

	/** Constraints on buffers to be transferred without copying */
	dma_policy_t transfer_buffer_policy;
} usb_pipe_desc_t;

typedef struct usb_pipe_transfer_request {
	usb_direction_t dir;
	usb_endpoint_t endpoint;
	usb_stream_t stream;

	uint64_t setup;			/**< Valid iff the transfer is of control type */

	/**
	 * The DMA buffer to share. Must be at least offset + size large. Is
	 * patched after being transmitted over IPC, so the pointer is still
	 * valid.
	 */
	dma_buffer_t buffer;
	size_t offset;			/**< Offset to the buffer */
	size_t size;			/**< Requested size. */
} usbhc_iface_transfer_request_t;

/** This structure follows standard endpoint descriptor + superspeed companion
 * descriptor, and exists to avoid dependency of libdrv on libusb. Keep the
 * internal fields named exactly like their source (because we want to use the
 * same macros to access them).
 * Callers shall fill it with bare contents of respective descriptors (in usb endianity).
 */
typedef struct usb_endpoint_descriptors {
	struct {
		uint8_t endpoint_address;
		uint8_t attributes;
		uint16_t max_packet_size;
		uint8_t poll_interval;
	} endpoint;

	/* Superspeed companion descriptor */
	struct companion_desc_t {
		uint8_t max_burst;
		uint8_t attributes;
		uint16_t bytes_per_interval;
	} companion;
} usb_endpoint_descriptors_t;

extern errno_t usbhc_reserve_default_address(async_exch_t *);
extern errno_t usbhc_release_default_address(async_exch_t *);

extern errno_t usbhc_device_enumerate(async_exch_t *, unsigned, usb_speed_t);
extern errno_t usbhc_device_remove(async_exch_t *, unsigned);

extern errno_t usbhc_register_endpoint(async_exch_t *, usb_pipe_desc_t *, const usb_endpoint_descriptors_t *);
extern errno_t usbhc_unregister_endpoint(async_exch_t *, const usb_pipe_desc_t *);

extern errno_t usbhc_transfer(async_exch_t *, const usbhc_iface_transfer_request_t *, size_t *);

/** Callback for outgoing transfer */
typedef errno_t (*usbhc_iface_transfer_callback_t)(void *, int, size_t);

/** USB device communication interface. */
typedef struct {
	errno_t (*default_address_reservation)(ddf_fun_t *, bool);

	errno_t (*device_enumerate)(ddf_fun_t *, unsigned, usb_speed_t);
	errno_t (*device_remove)(ddf_fun_t *, unsigned);

	errno_t (*register_endpoint)(ddf_fun_t *, usb_pipe_desc_t *, const usb_endpoint_descriptors_t *);
	errno_t (*unregister_endpoint)(ddf_fun_t *, const usb_pipe_desc_t *);

	errno_t (*transfer)(ddf_fun_t *, const usbhc_iface_transfer_request_t *,
	    usbhc_iface_transfer_callback_t, void *);
} usbhc_iface_t;

#endif
/**
 * @}
 */
