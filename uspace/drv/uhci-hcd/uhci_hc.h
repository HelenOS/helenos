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
#ifndef DRV_UHCI_UHCI_HC_H
#define DRV_UHCI_UHCI_HC_H

#include <fibril.h>
#include <fibril_synch.h>
#include <adt/list.h>
#include <ddi.h>

#include <usbhc_iface.h>

#include "batch.h"
#include "transfer_list.h"
#include "utils/device_keeper.h"

typedef struct uhci_regs {
	uint16_t usbcmd;
#define UHCI_CMD_MAX_PACKET (1 << 7)
#define UHCI_CMD_CONFIGURE  (1 << 6)
#define UHCI_CMD_DEBUG  (1 << 5)
#define UHCI_CMD_FORCE_GLOBAL_RESUME  (1 << 4)
#define UHCI_CMD_FORCE_GLOBAL_SUSPEND  (1 << 3)
#define UHCI_CMD_GLOBAL_RESET  (1 << 2)
#define UHCI_CMD_HCRESET  (1 << 1)
#define UHCI_CMD_RUN_STOP  (1 << 0)

	uint16_t usbsts;
#define UHCI_STATUS_HALTED (1 << 5)
#define UHCI_STATUS_PROCESS_ERROR (1 << 4)
#define UHCI_STATUS_SYSTEM_ERROR (1 << 3)
#define UHCI_STATUS_RESUME (1 << 2)
#define UHCI_STATUS_ERROR_INTERRUPT (1 << 1)
#define UHCI_STATUS_INTERRUPT (1 << 0)

	uint16_t usbintr;
#define UHCI_INTR_SHORT_PACKET (1 << 3)
#define UHCI_INTR_COMPLETE (1 << 2)
#define UHCI_INTR_RESUME (1 << 1)
#define UHCI_INTR_CRC (1 << 0)

	uint16_t frnum;
	uint32_t flbaseadd;
	uint8_t sofmod;
} regs_t;

#define UHCI_FRAME_LIST_COUNT 1024
#define UHCI_CLEANER_TIMEOUT 10000
#define UHCI_DEBUGER_TIMEOUT 5000000
#define UHCI_ALLOWED_HW_FAIL 5

typedef struct uhci_hc {
	device_keeper_t device_manager;

	regs_t *registers;

	link_pointer_t *frame_list;

	transfer_list_t transfers_bulk_full;
	transfer_list_t transfers_control_full;
	transfer_list_t transfers_control_slow;
	transfer_list_t transfers_interrupt;

	transfer_list_t *transfers[2][4];

	irq_code_t interrupt_code;

	fid_t cleaner;
	fid_t debug_checker;
	bool hw_interrupts;
	unsigned hw_failures;

	ddf_fun_t *ddf_instance;
} uhci_hc_t;

int uhci_hc_init(uhci_hc_t *instance, ddf_fun_t *fun,
    void *regs, size_t reg_size, bool interupts);

int uhci_hc_schedule(uhci_hc_t *instance, batch_t *batch);

void uhci_hc_interrupt(uhci_hc_t *instance, uint16_t status);

/** Safely dispose host controller internal structures
 *
 * @param[in] instance Host controller structure to use.
 */
static inline void uhci_hc_fini(uhci_hc_t *instance) { /* TODO: implement*/ };

/** Get and cast pointer to the driver data
 *
 * @param[in] fun DDF function pointer
 * @return cast pointer to driver_data
 */
static inline uhci_hc_t * fun_to_uhci_hc(ddf_fun_t *fun)
	{ return (uhci_hc_t*)fun->driver_data; }
#endif
/**
 * @}
 */
