/*
 * Copyright (c) 2010 Matus Dekanek
 * Copyright (c) 2018 Ondrej Hlavaty
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
 * @brief USB hub related structures.
 */
#ifndef LIBUSB_CLASS_HUB_H_
#define LIBUSB_CLASS_HUB_H_

#include <stdint.h>

/** Hub class feature selector.
 * @warning The constants are not unique (feature selectors are used
 * for hub and port).
 */
typedef enum {
	USB_HUB_FEATURE_C_HUB_LOCAL_POWER = 0,
	USB_HUB_FEATURE_C_HUB_OVER_CURRENT = 1,
	USB_HUB_FEATURE_HUB_LOCAL_POWER = 0,
	USB_HUB_FEATURE_HUB_OVER_CURRENT = 1,
	USB_HUB_FEATURE_PORT_CONNECTION = 0,
	USB2_HUB_FEATURE_PORT_ENABLE = 1,
	USB2_HUB_FEATURE_PORT_SUSPEND = 2,
	USB_HUB_FEATURE_PORT_OVER_CURRENT = 3,
	USB_HUB_FEATURE_PORT_RESET = 4,
	USB3_HUB_FEATURE_PORT_LINK_STATE = 5,
	USB_HUB_FEATURE_PORT_POWER = 8,
	USB2_HUB_FEATURE_PORT_LOW_SPEED = 9,
	USB_HUB_FEATURE_C_PORT_CONNECTION = 16,
	USB2_HUB_FEATURE_C_PORT_ENABLE = 17,
	USB2_HUB_FEATURE_C_PORT_SUSPEND = 18,
	USB_HUB_FEATURE_C_PORT_OVER_CURRENT = 19,
	USB_HUB_FEATURE_C_PORT_RESET = 20,
	USB2_HUB_FEATURE_PORT_TEST = 21,
	USB2_HUB_FEATURE_PORT_INDICATOR = 22,
	USB3_HUB_FEATURE_C_PORT_LINK_STATE = 25,
	USB3_HUB_FEATURE_BH_PORT_RESET = 28,
	USB3_HUB_FEATURE_C_BH_PORT_RESET = 29,
	/* USB_HUB_FEATURE_ = , */
} usb_hub_class_feature_t;

/**
 * Dword holding port status and changes flags.
 *
 * For more information refer to tables 11-15 and 11-16 in
 * "Universal Serial Bus Specification Revision 1.1" pages 274 and 277
 * (290 and 293 in pdf)
 *
 * Beware that definition of bits changed between USB 2 and 3,
 * so some fields are prefixed with USB2 or USB3 instead.
 */
typedef uint32_t usb_port_status_t;

#define USB_HUB_PORT_STATUS_BIT(bit)  (uint32_usb2host(1 << (bit)))
#define USB_HUB_PORT_STATUS_CONNECTION		USB_HUB_PORT_STATUS_BIT(0)
#define USB_HUB_PORT_STATUS_ENABLE		USB_HUB_PORT_STATUS_BIT(1)
#define USB2_HUB_PORT_STATUS_SUSPEND		USB_HUB_PORT_STATUS_BIT(2)
#define USB_HUB_PORT_STATUS_OC			USB_HUB_PORT_STATUS_BIT(3)
#define USB_HUB_PORT_STATUS_RESET		USB_HUB_PORT_STATUS_BIT(4)

#define USB2_HUB_PORT_STATUS_POWER		USB_HUB_PORT_STATUS_BIT(8)
#define USB2_HUB_PORT_STATUS_LOW_SPEED		USB_HUB_PORT_STATUS_BIT(9)
#define USB3_HUB_PORT_STATUS_POWER		USB_HUB_PORT_STATUS_BIT(9)
#define USB2_HUB_PORT_STATUS_HIGH_SPEED		USB_HUB_PORT_STATUS_BIT(10)
#define USB2_HUB_PORT_STATUS_TEST		USB_HUB_PORT_STATUS_BIT(11)
#define USB2_HUB_PORT_STATUS_INDICATOR		USB_HUB_PORT_STATUS_BIT(12)

#define USB_HUB_PORT_STATUS_C_CONNECTION	USB_HUB_PORT_STATUS_BIT(16)
#define USB2_HUB_PORT_STATUS_C_ENABLE		USB_HUB_PORT_STATUS_BIT(17)
#define USB2_HUB_PORT_STATUS_C_SUSPEND		USB_HUB_PORT_STATUS_BIT(18)
#define USB_HUB_PORT_STATUS_C_OC		USB_HUB_PORT_STATUS_BIT(19)
#define USB_HUB_PORT_STATUS_C_RESET		USB_HUB_PORT_STATUS_BIT(20)
#define USB3_HUB_PORT_STATUS_C_BH_RESET		USB_HUB_PORT_STATUS_BIT(21)
#define USB3_HUB_PORT_STATUS_C_LINK_STATE	USB_HUB_PORT_STATUS_BIT(22)
#define USB3_HUB_PORT_STATUS_C_CONFIG_ERROR	USB_HUB_PORT_STATUS_BIT(23)

/** Header of standard hub descriptor without the "variadic" part. */
typedef struct {
	/** Descriptor length. */
	uint8_t length;

	/** Descriptor type (0x29 or 0x2a for superspeed hub). */
	uint8_t descriptor_type;

	/** Number of downstream ports. */
	uint8_t port_count;

	/** Characteristics bitmask.
	 *
	 *  D1..D0: Logical Power Switching Mode
	 *  00: Ganged power switching (all ports power at
	 *  once)
	 *  01: Individual port power switching
	 *  1X: Reserved. Used only on 1.0 compliant hubs
	 *  that implement no power switching.
	 *  D2: Identifies a Compound Device
	 *  0: Hub is not part of a compound device
	 *  1: Hub is part of a compound device
	 *  D4..D3: Over-current Protection Mode
	 *  00: Global Over-current Protection. The hub
	 *  reports over-current as a summation of all
	 *  ports current draw, without a breakdown of
	 *  individual port over-current status.
	 *  01: Individual Port Over-current Protection. The
	 *  hub reports over-current on a per-port basis.
	 *  Each port has an over-current indicator.
	 *  1X: No Over-current Protection. This option is
	 *  allowed only for bus-powered hubs that do not
	 *  implement over-current protection.
	 *  D6..D5: TT think time
	 *  00: At most 8 FS bit times
	 *  01: At most 16 FS bit times
	 *  10: At most 24 FS bit times
	 *  11: At most 32 FS bit times
	 *  D7: Port indicators
	 *  0: Not supported
	 *  1: Supported
	 *  D15...D8: Reserved
	 */
	uint8_t characteristics;

	/** Unused part of characteristics field */
	uint8_t characteristics_reserved;

	/** Time from power-on to stabilization of current on the port.
	 *
	 *  Time (in 2ms intervals) from the time the power-on
	 *  sequence begins on a port until power is good on that
	 *  port. The USB System Software uses this value to
	 *  determine how long to wait before accessing a
	 *  powered-on port.
	 */
	uint8_t power_good_time;
	/** Maximum current requirements in mA.
	 *
	 *  Maximum current requirements of the Hub Controller
	 *  electronics in mA.
	 */
	uint8_t max_current;
} __attribute__((packed)) usb_hub_descriptor_header_t;

/*
 * USB hub characteristics
 */
#define HUB_CHAR_POWER_PER_PORT_FLAG    (1 << 0)
#define HUB_CHAR_NO_POWER_SWITCH_FLAG   (1 << 1)
#define HUB_CHAR_COMPOUND_DEVICE        (1 << 2)
#define HUB_CHAR_OC_PER_PORT_FLAG       (1 << 3)
#define HUB_CHAR_NO_OC_FLAG             (1 << 4)

/* These are invalid for superspeed hub */
#define HUB_CHAR_TT_THINK_16            (1 << 5)
#define HUB_CHAR_TT_THINK_8             (1 << 6)
#define HUB_CHAR_INDICATORS_FLAG        (1 << 7)

/** One bit for the device and one bit for every port */
#define STATUS_BYTES(ports) ((1 + ports + 7) / 8)

/**
 * @brief usb hub specific request types.
 */
typedef enum {
	/** This request resets a value reported in the hub status. */
	USB_HUB_REQ_TYPE_CLEAR_HUB_FEATURE = 0x20,
	/** This request resets a value reported in the port status. */
	USB_HUB_REQ_TYPE_CLEAR_PORT_FEATURE = 0x23,
	/**
	 * This is an optional per-port diagnostic request that returns the bus
	 * state value, as sampled at the last EOF2 point.
	 */
	USB_HUB_REQ_TYPE_GET_STATE = 0xA3,
	/** This request returns the hub descriptor. */
	USB_HUB_REQ_TYPE_GET_DESCRIPTOR = 0xA0,
	/**
	 * This request returns the current hub status and the states that have
	 * changed since the previous acknowledgment.
	 */
	USB_HUB_REQ_TYPE_GET_HUB_STATUS = 0xA0,
	/**
	 * This request returns the current port status and the current value of the
	 * port status change bits.
	 */
	USB_HUB_REQ_TYPE_GET_PORT_STATUS = 0xA3,
	/** This request overwrites the hub descriptor. */
	USB_HUB_REQ_TYPE_SET_DESCRIPTOR = 0x20,
	/** This request sets a value reported in the hub status. */
	USB_HUB_REQ_TYPE_SET_HUB_FEATURE = 0x20,
	/**
	 * This request sets the value that the hub uses to determine the index
	 * into the Route String Index for the hub.
	 */
	USB_HUB_REQ_TYPE_SET_HUB_DEPTH = 0x20,
	/** This request sets a value reported in the port status. */
	USB_HUB_REQ_TYPE_SET_PORT_FEATURE = 0x23,
} usb_hub_bm_request_type_t;

/**
 * @brief hub class request codes
 */
typedef enum {
	USB_HUB_REQUEST_GET_STATUS = 0,
	USB_HUB_REQUEST_CLEAR_FEATURE = 1,
	/** USB 1.0 only */
	USB_HUB_REQUEST_GET_STATE = 2,
	USB_HUB_REQUEST_SET_FEATURE = 3,
	USB_HUB_REQUEST_GET_DESCRIPTOR = 6,
	USB_HUB_REQUEST_SET_DESCRIPTOR = 7,
	USB_HUB_REQUEST_CLEAR_TT_BUFFER = 8,
	USB_HUB_REQUEST_RESET_TT = 9,
	USB_HUB_GET_TT_STATE = 10,
	USB_HUB_STOP_TT = 11,
	/** USB 3+ only */
	USB_HUB_REQUEST_SET_HUB_DEPTH = 12,
} usb_hub_request_t;

/**
 * Maximum size of usb hub descriptor in bytes
 */
/* 7 (basic size) + 2*32 (port bitmasks) */
#define USB_HUB_MAX_DESCRIPTOR_SIZE  (7 + 2 * 32)

#endif
/**
 * @}
 */
