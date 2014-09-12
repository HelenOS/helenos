/*
 * Copyright (c) 2011 Jan Vesely
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

/** @addtogroup drvusbuhcihc
 * @{
 */
/** @file
 * @brief UHCI host controller driver structure
 */
#ifndef DRV_UHCI_HC_H
#define DRV_UHCI_HC_H

#include <ddf/driver.h>
#include <ddf/interrupt.h>
#include <device/hw_res_parsed.h>
#include <fibril.h>
#include <usb/host/hcd.h>

#include "transfer_list.h"

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
} uhci_regs_t;

#define UHCI_FRAME_LIST_COUNT 1024
#define UHCI_INT_EMULATOR_TIMEOUT 10000
#define UHCI_DEBUGER_TIMEOUT 5000000
#define UHCI_ALLOWED_HW_FAIL 5
#define UHCI_NEEDED_IRQ_COMMANDS 5

/** Main UHCI driver structure */
typedef struct hc {
	/** Generic HCD driver structure */
	hcd_t *generic;

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
	/** Fibril periodically checking status register*/
	fid_t interrupt_emulator;
	/** Indicator of hw interrupts availability */
	bool hw_interrupts;

	/** Number of hw failures detected. */
	unsigned hw_failures;
} hc_t;

int hc_register_irq_handler(ddf_dev_t *, addr_range_t *, int,
    interrupt_handler_t);
int hc_get_irq_code(irq_pio_range_t [], size_t, irq_cmd_t [], size_t,
    addr_range_t *);
void hc_interrupt(hc_t *instance, uint16_t status);
int hc_init(hc_t *, ddf_fun_t *, addr_range_t *, bool);

/** Safely dispose host controller internal structures
 *
 * @param[in] instance Host controller structure to use.
 */
static inline void hc_fini(hc_t *instance) {} /* TODO: implement*/
#endif

/**
 * @}
 */
