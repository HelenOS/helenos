/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbuhci
 * @{
 */
/** @file
 * @brief UHCI host controller driver structure
 */

#ifndef DRV_UHCI_HC_H
#define DRV_UHCI_HC_H

#include <device/hw_res_parsed.h>
#include <fibril.h>
#include <macros.h>
#include <member.h>
#include <stdbool.h>
#include <ddi.h>
#include <usb/host/hcd.h>
#include <usb/host/usb2_bus.h>
#include <usb/host/usb_transfer_batch.h>

#include "uhci_rh.h"
#include "transfer_list.h"
#include "hw_struct/link_pointer.h"

/** UHCI I/O registers layout */
typedef struct uhci_regs {
	/** Command register, controls HC behaviour */
	ioport16_t usbcmd;
#define UHCI_CMD_MAX_PACKET (1 << 7)
#define UHCI_CMD_CONFIGURE  (1 << 6)
#define UHCI_CMD_DEBUG  (1 << 5)
#define UHCI_CMD_FORCE_GLOBAL_RESUME  (1 << 4)
#define UHCI_CMD_FORCE_GLOBAL_SUSPEND  (1 << 3)
#define UHCI_CMD_GLOBAL_RESET  (1 << 2)
#define UHCI_CMD_HCRESET  (1 << 1)
#define UHCI_CMD_RUN_STOP  (1 << 0)

	/** Status register, 1 means interrupt is asserted (if enabled) */
	ioport16_t usbsts;
#define UHCI_STATUS_HALTED (1 << 5)
#define UHCI_STATUS_PROCESS_ERROR (1 << 4)
#define UHCI_STATUS_SYSTEM_ERROR (1 << 3)
#define UHCI_STATUS_RESUME (1 << 2)
#define UHCI_STATUS_ERROR_INTERRUPT (1 << 1)
#define UHCI_STATUS_INTERRUPT (1 << 0)
#define UHCI_STATUS_NM_INTERRUPTS \
    (UHCI_STATUS_PROCESS_ERROR | UHCI_STATUS_SYSTEM_ERROR)

	/** Interrupt enabled registers */
	ioport16_t usbintr;
#define UHCI_INTR_SHORT_PACKET (1 << 3)
#define UHCI_INTR_COMPLETE (1 << 2)
#define UHCI_INTR_RESUME (1 << 1)
#define UHCI_INTR_CRC (1 << 0)

	/** Register stores frame number used in SOF packet */
	ioport16_t frnum;

	/** Pointer(physical) to the Frame List */
	ioport32_t flbaseadd;

	/** SOF modification to match external timers */
	ioport8_t sofmod;

	PADD8(3);
	ioport16_t ports[];
} uhci_regs_t;

#define UHCI_FRAME_LIST_COUNT 1024
#define UHCI_DEBUGER_TIMEOUT 5000000
#define UHCI_ALLOWED_HW_FAIL 5

/** Main UHCI driver structure */
typedef struct hc {
	/* Common hc_device header */
	hc_device_t base;

	uhci_rh_t rh;
	bus_t bus;
	usb2_bus_helper_t bus_helper;

	/** Addresses of I/O registers */
	uhci_regs_t *registers;

	/** Frame List contains 1024 link pointers */
	link_pointer_t *frame_list;

	/** List and queue of interrupt transfers */
	transfer_list_t transfers_interrupt;
	/** List and queue of low speed control transfers */
	transfer_list_t transfers_control_slow;
	/** List and queue of full speed bulk transfers */
	transfer_list_t transfers_bulk_full;
	/** List and queue of full speed control transfers */
	transfer_list_t transfers_control_full;

	/** Pointer table to the above lists, helps during scheduling */
	transfer_list_t *transfers[2][4];

	/**
	 * Guard for the pending list. Can be locked under EP guard, but not
	 * vice versa.
	 */
	fibril_mutex_t guard;
	/** List of endpoints with a transfer scheduled */
	list_t pending_endpoints;

	/** Number of hw failures detected. */
	unsigned hw_failures;
} hc_t;

typedef struct uhci_endpoint {
	endpoint_t base;

	bool toggle;
} uhci_endpoint_t;

static inline hc_t *hcd_to_hc(hc_device_t *hcd)
{
	assert(hcd);
	return (hc_t *) hcd;
}

static inline hc_t *bus_to_hc(bus_t *bus)
{
	assert(bus);
	return member_to_inst(bus, hc_t, bus);
}

int hc_unschedule_batch(usb_transfer_batch_t *);

extern errno_t hc_add(hc_device_t *, const hw_res_list_parsed_t *);
extern errno_t hc_gen_irq_code(irq_code_t *, hc_device_t *, const hw_res_list_parsed_t *, int *);
extern errno_t hc_start(hc_device_t *);
extern errno_t hc_setup_roothub(hc_device_t *);
extern errno_t hc_gone(hc_device_t *);

#endif

/**
 * @}
 */
