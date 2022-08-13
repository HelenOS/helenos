/*
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty, Jan Hrach, Jaroslav Jindrak, Petr Manek
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbxhci
 * @{
 */
/** @file
 * @brief The host controller data bookkeeping.
 */

#ifndef XHCI_HC_H
#define XHCI_HC_H

#include <fibril_synch.h>
#include <member.h>
#include <usb/host/usb_transfer_batch.h>
#include <usb/host/utility.h>
#include "hw_struct/regs.h"
#include "hw_struct/context.h"
#include "scratchpad.h"
#include "trb_ring.h"

#include "rh.h"
#include "commands.h"
#include "bus.h"

typedef struct xhci_command xhci_cmd_t;

typedef struct xhci_hc {
	/** Common HC device header */
	hc_device_t base;

	/* MMIO range */
	addr_range_t mmio_range;

	/* Mapped register sets */
	void *reg_base;
	xhci_cap_regs_t *cap_regs;
	xhci_op_regs_t *op_regs;
	xhci_rt_regs_t *rt_regs;
	xhci_doorbell_t *db_arry;
	xhci_extcap_t *xecp;		/**< First extended capability */
	xhci_legsup_t *legsup;		/**< Legacy support capability */

	/* Structures in allocated memory */
	xhci_event_ring_t event_ring;
	uint64_t *dcbaa;
	dma_buffer_t dcbaa_dma;
	dma_buffer_t scratchpad_array;

	/* Command ring management */
	xhci_cmd_ring_t cr;

	/* Buffer for events */
	xhci_sw_ring_t sw_ring;

	/** Event handling fibril */
	joinable_fibril_t *event_worker;

	/* Root hub emulation */
	xhci_rh_t rh;

	/* Bus bookkeeping */
	xhci_bus_t bus;

	/* Fibril that is currently hanling events */
	fid_t event_handler;

	/* Cached capabilities */
	unsigned max_slots;
	bool ac64;
	bool csz;
	uint64_t wrap_time;	/**< The last time when mfindex wrap happened */
	uint64_t wrap_count;	/**< Amount of mfindex wraps HC has done */
	unsigned ist;		/**< IST in microframes */

	/** Port speed mapping */
	xhci_port_speed_t speeds [16];
} xhci_hc_t;

static inline xhci_hc_t *bus_to_hc(bus_t *bus)
{
	assert(bus);
	return member_to_inst(bus, xhci_hc_t, bus);
}

typedef struct xhci_endpoint xhci_endpoint_t;
typedef struct xhci_device xhci_device_t;

extern errno_t hc_init_mmio(xhci_hc_t *, const hw_res_list_parsed_t *);
extern errno_t hc_init_memory(xhci_hc_t *, ddf_dev_t *);
extern errno_t hc_claim(xhci_hc_t *, ddf_dev_t *);
extern errno_t hc_irq_code_gen(irq_code_t *, xhci_hc_t *, const hw_res_list_parsed_t *, int *);
extern errno_t hc_start(xhci_hc_t *);
extern void hc_fini(xhci_hc_t *);

extern void hc_ring_doorbell(xhci_hc_t *, unsigned, unsigned);
extern void hc_ring_ep_doorbell(xhci_endpoint_t *, uint32_t);
extern unsigned hc_speed_to_psiv(usb_speed_t);

extern errno_t hc_enable_slot(xhci_device_t *);
extern errno_t hc_disable_slot(xhci_device_t *);
extern errno_t hc_address_device(xhci_device_t *);
extern errno_t hc_configure_device(xhci_device_t *);
extern errno_t hc_deconfigure_device(xhci_device_t *);
extern errno_t hc_add_endpoint(xhci_endpoint_t *);
extern errno_t hc_drop_endpoint(xhci_endpoint_t *);
extern errno_t hc_update_endpoint(xhci_endpoint_t *);
extern errno_t hc_stop_endpoint(xhci_endpoint_t *);
extern errno_t hc_reset_endpoint(xhci_endpoint_t *);
extern errno_t hc_reset_ring(xhci_endpoint_t *, uint32_t);

extern errno_t hc_status(bus_t *, uint32_t *);
extern void hc_interrupt(bus_t *, uint32_t);

#endif

/**
 * @}
 */
