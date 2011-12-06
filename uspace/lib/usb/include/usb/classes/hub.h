/*
 * Copyright (c) 2010 Matus Dekanek
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

#include <sys/types.h>

/** Hub class feature selector.
 * @warning The constants are not unique (feature selectors are used
 * for hub and port).
 */
typedef enum {
	USB_HUB_FEATURE_HUB_LOCAL_POWER = 0,
	USB_HUB_FEATURE_HUB_OVER_CURRENT = 1,
	USB_HUB_FEATURE_C_HUB_LOCAL_POWER = 0,
	USB_HUB_FEATURE_C_HUB_OVER_CURRENT = 1,
	USB_HUB_FEATURE_PORT_CONNECTION = 0,
	USB_HUB_FEATURE_PORT_ENABLE = 1,
	USB_HUB_FEATURE_PORT_SUSPEND = 2,
	USB_HUB_FEATURE_PORT_OVER_CURRENT = 3,
	USB_HUB_FEATURE_PORT_RESET = 4,
	USB_HUB_FEATURE_PORT_POWER = 8,
	USB_HUB_FEATURE_PORT_LOW_SPEED = 9,
	USB_HUB_FEATURE_C_PORT_CONNECTION = 16,
	USB_HUB_FEATURE_C_PORT_ENABLE = 17,
	USB_HUB_FEATURE_C_PORT_SUSPEND = 18,
	USB_HUB_FEATURE_C_PORT_OVER_CURRENT = 19,
	USB_HUB_FEATURE_C_PORT_RESET = 20,
	/* USB_HUB_FEATURE_ = , */
} usb_hub_class_feature_t;


/** Header of standard hub descriptor without the "variadic" part. */
typedef struct {
	/** Descriptor length. */
	uint8_t length;
	/** Descriptor type (0x29). */
	uint8_t descriptor_type;
	/** Number of downstream ports. */
	uint8_t port_count;
	/** Characteristics bitmask. */
	uint8_t characteristics;
#define HUB_CHAR_POWER_PER_PORT_FLAG  (1 << 0)
#define HUB_CHAR_NO_POWER_SWITCH_FLAG (1 << 1)
	/* Unused part of characteristics field */
	uint8_t characteristics_reserved;
	/** Time from power-on to stabilization of current on the port. */
	uint8_t power_good_time;
	/** Maximum current requirements in mA. */
	uint8_t max_current;
} __attribute__ ((packed)) usb_hub_descriptor_header_t;

/**
 * @brief usb hub descriptor
 *
 * For more information see Universal Serial Bus Specification Revision 1.1
 * chapter 11.16.2
 */
typedef struct usb_hub_descriptor_type {
    /** Number of bytes in this descriptor, including this byte */
    //uint8_t bDescLength;

    /** Descriptor Type, value: 29H for hub descriptor */
    //uint8_t bDescriptorType;

    /** Number of downstream ports that this hub supports */
    uint8_t port_count;

    /**
            D1...D0: Logical Power Switching Mode
            00: Ganged power switching (all ports power at
            once)
            01: Individual port power switching
            1X: Reserved. Used only on 1.0 compliant hubs
            that implement no power switching.
            D2: Identifies a Compound Device
            0: Hub is not part of a compound device
            1: Hub is part of a compound device
            D4...D3: Over-current Protection Mode
            00: Global Over-current Protection. The hub
            reports over-current as a summation of all
            ports current draw, without a breakdown of
            individual port over-current status.
            01: Individual Port Over-current Protection. The
            hub reports over-current on a per-port basis.
            Each port has an over-current indicator.
            1X: No Over-current Protection. This option is
            allowed only for bus-powered hubs that do not
            implement over-current protection.
            D15...D5:
            Reserved
     */
    uint16_t hub_characteristics;

    /**
            Time (in 2ms intervals) from the time the power-on
            sequence begins on a port until power is good on that
            port. The USB System Software uses this value to
            determine how long to wait before accessing a
            powered-on port.
     */
    uint8_t pwr_on_2_good_time;

    /**
            Maximum current requirements of the Hub Controller
            electronics in mA.
     */
    uint8_t current_requirement;

    /**
            Indicates if a port has a removable device attached.
            This field is reported on byte-granularity. Within a
            byte, if no port exists for a given location, the field
            representing the port characteristics returns 0.
            Bit value definition:
            0B - Device is removable
            1B - Device is non-removable
            This is a bitmap corresponding to the individual ports
            on the hub:
            Bit 0: Reserved for future use
            Bit 1: Port 1
            Bit 2: Port 2
            ....
            Bit n: Port n (implementation-dependent, up to a
            maximum of 255 ports).
     */
    uint8_t devices_removable[32];

    /**
            This field exists for reasons of compatibility with
            software written for 1.0 compliant devices. All bits in
            this field should be set to 1B. This field has one bit for
            each port on the hub with additional pad bits, if
            necessary, to make the number of bits in the field an
            integer multiple of 8.
     */
    //uint8_t * port_pwr_ctrl_mask;
} usb_hub_descriptor_t;



/**	@brief usb hub specific request types.
 *
 *	For more information see Universal Serial Bus Specification Revision 1.1 chapter 11.16.2
 */
typedef enum {
    /**	This request resets a value reported in the hub status.	*/
    USB_HUB_REQ_TYPE_CLEAR_HUB_FEATURE = 0x20,
    /** This request resets a value reported in the port status. */
    USB_HUB_REQ_TYPE_CLEAR_PORT_FEATURE = 0x23,
    /** This is an optional per-port diagnostic request that returns the bus state value, as sampled at the last EOF2 point. */
    USB_HUB_REQ_TYPE_GET_STATE = 0xA3,
    /** This request returns the hub descriptor. */
    USB_HUB_REQ_TYPE_GET_DESCRIPTOR = 0xA0,
    /** This request returns the current hub status and the states that have changed since the previous acknowledgment. */
    USB_HUB_REQ_TYPE_GET_HUB_STATUS = 0xA0,
    /** This request returns the current port status and the current value of the port status change bits. */
    USB_HUB_REQ_TYPE_GET_PORT_STATUS = 0xA3,
    /** This request overwrites the hub descriptor. */
    USB_HUB_REQ_TYPE_SET_DESCRIPTOR = 0x20,
    /** This request sets a value reported in the hub status. */
    USB_HUB_REQ_TYPE_SET_HUB_FEATURE = 0x20,
    /** This request sets a value reported in the port status. */
    USB_HUB_REQ_TYPE_SET_PORT_FEATURE = 0x23
} usb_hub_bm_request_type_t;

/** @brief hub class request codes*/
/// \TODO these are duplicit to standart descriptors
typedef enum {
    /**  */
    USB_HUB_REQUEST_GET_STATUS = 0,
    /** */
    USB_HUB_REQUEST_CLEAR_FEATURE = 1,
    /** */
    USB_HUB_REQUEST_GET_STATE = 2,
    /** */
    USB_HUB_REQUEST_SET_FEATURE = 3,
    /** */
    USB_HUB_REQUEST_GET_DESCRIPTOR = 6,
    /** */
    USB_HUB_REQUEST_SET_DESCRIPTOR = 7
} usb_hub_request_t;

/**
 *	Maximum size of usb hub descriptor in bytes
 */
/* 7 (basic size) + 2*32 (port bitmasks) */
#define USB_HUB_MAX_DESCRIPTOR_SIZE 71

#endif
/**
 * @}
 */
