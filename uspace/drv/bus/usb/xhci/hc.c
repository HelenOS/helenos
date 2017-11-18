/*
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

/** @addtogroup drvusbxhci
 * @{
 */
/** @file
 * @brief The host controller data bookkeeping.
 */

#include <errno.h>
#include <str_error.h>
#include <usb/debug.h>
#include <usb/host/endpoint.h>
#include "debug.h"
#include "hc.h"
#include "rh.h"
#include "hw_struct/trb.h"
#include "hw_struct/context.h"
#include "endpoint.h"
#include "commands.h"
#include "transfers.h"
#include "trb_ring.h"

/**
 * Default USB Speed ID mapping: Table 157
 */
#define PSI_TO_BPS(psie, psim) (((uint64_t) psim) << (10 * psie))
#define PORT_SPEED(usb, mjr, psie, psim) { \
	.name = "USB ", \
	.major = mjr, \
	.minor = 0, \
	.usb_speed = USB_SPEED_##usb, \
	.rx_bps = PSI_TO_BPS(psie, psim), \
	.tx_bps = PSI_TO_BPS(psie, psim) \
}
static const xhci_port_speed_t ps_default_full  = PORT_SPEED(FULL, 2, 2, 12);
static const xhci_port_speed_t ps_default_low   = PORT_SPEED(LOW, 2, 1, 1500);
static const xhci_port_speed_t ps_default_high  = PORT_SPEED(HIGH, 2, 2, 480);
static const xhci_port_speed_t ps_default_super = PORT_SPEED(SUPER, 3, 3, 5);

/**
 * Walk the list of extended capabilities.
 */
static int hc_parse_ec(xhci_hc_t *hc)
{
	unsigned psic, major, minor;
	xhci_sp_name_t name;

	xhci_port_speed_t *speeds = hc->speeds;

	for (xhci_extcap_t *ec = hc->xecp; ec; ec = xhci_extcap_next(ec)) {
		xhci_dump_extcap(ec);
		switch (XHCI_REG_RD(ec, XHCI_EC_CAP_ID)) {
		case XHCI_EC_USB_LEGACY:
			assert(hc->legsup == NULL);
			hc->legsup = (xhci_legsup_t *) ec;
			break;
		case XHCI_EC_SUPPORTED_PROTOCOL:
			psic = XHCI_REG_RD(ec, XHCI_EC_SP_PSIC);
			major = XHCI_REG_RD(ec, XHCI_EC_SP_MAJOR);
			minor = XHCI_REG_RD(ec, XHCI_EC_SP_MINOR);
			name.packed = host2uint32_t_le(XHCI_REG_RD(ec, XHCI_EC_SP_NAME));

			if (name.packed != xhci_name_usb.packed) {
				/**
				 * The detection of such protocol would work,
				 * but the rest of the implementation is made
				 * for the USB protocol only.
				 */
				usb_log_error("Unknown protocol %.4s.", name.str);
				return ENOTSUP;
			}

			// "Implied" speed
			if (psic == 0) {
				assert(minor == 0);

				if (major == 2) {
					speeds[1] = ps_default_full;
					speeds[2] = ps_default_low;
					speeds[3] = ps_default_high;

					hc->speed_to_psiv[USB_SPEED_FULL] = 1;
					hc->speed_to_psiv[USB_SPEED_LOW] = 2;
					hc->speed_to_psiv[USB_SPEED_HIGH] = 3;
				} else if (major == 3) {
					speeds[4] = ps_default_super;
					hc->speed_to_psiv[USB_SPEED_SUPER] = 4;
				} else {
					return EINVAL;
				}

				usb_log_debug2("Implied speed of USB %u.0 set up.", major);
			} else {
				for (unsigned i = 0; i < psic; i++) {
					xhci_psi_t *psi = xhci_extcap_psi(ec, i);
					unsigned sim = XHCI_REG_RD(psi, XHCI_PSI_PSIM);
					unsigned psiv = XHCI_REG_RD(psi, XHCI_PSI_PSIV);
					unsigned psie = XHCI_REG_RD(psi, XHCI_PSI_PSIE);
					unsigned psim = XHCI_REG_RD(psi, XHCI_PSI_PSIM);

					speeds[psiv].major = major;
					speeds[psiv].minor = minor;
					str_ncpy(speeds[psiv].name, 4, name.str, 4);
					speeds[psiv].usb_speed = USB_SPEED_MAX;

					uint64_t bps = PSI_TO_BPS(psie, psim);

					if (sim == XHCI_PSI_PLT_SYMM || sim == XHCI_PSI_PLT_RX)
						speeds[psiv].rx_bps = bps;
					if (sim == XHCI_PSI_PLT_SYMM || sim == XHCI_PSI_PLT_TX) {
						speeds[psiv].tx_bps = bps;
						usb_log_debug2("Speed %u set up for bps %" PRIu64 " / %" PRIu64 ".", psiv, speeds[psiv].rx_bps, speeds[psiv].tx_bps);
					}
				}
			}
		}
	}
	return EOK;
}

int hc_init_mmio(xhci_hc_t *hc, const hw_res_list_parsed_t *hw_res)
{
	int err;

	if (hw_res->mem_ranges.count != 1) {
		usb_log_error("Unexpected MMIO area, bailing out.");
		return EINVAL;
	}

	hc->mmio_range = hw_res->mem_ranges.ranges[0];

	usb_log_debug("MMIO area at %p (size %zu), IRQ %d.\n",
	    RNGABSPTR(hc->mmio_range), RNGSZ(hc->mmio_range), hw_res->irqs.irqs[0]);

	if (RNGSZ(hc->mmio_range) < sizeof(xhci_cap_regs_t))
		return EOVERFLOW;

	void *base;
	if ((err = pio_enable_range(&hc->mmio_range, &base)))
		return err;

	hc->reg_base = base;
	hc->cap_regs = (xhci_cap_regs_t *)  base;
	hc->op_regs  = (xhci_op_regs_t *)  (base + XHCI_REG_RD(hc->cap_regs, XHCI_CAP_LENGTH));
	hc->rt_regs  = (xhci_rt_regs_t *)  (base + XHCI_REG_RD(hc->cap_regs, XHCI_CAP_RTSOFF));
	hc->db_arry  = (xhci_doorbell_t *) (base + XHCI_REG_RD(hc->cap_regs, XHCI_CAP_DBOFF));

	uintptr_t xec_offset = XHCI_REG_RD(hc->cap_regs, XHCI_CAP_XECP) * sizeof(xhci_dword_t);
	if (xec_offset > 0)
		hc->xecp = (xhci_extcap_t *) (base + xec_offset);

	usb_log_debug2("Initialized MMIO reg areas:");
	usb_log_debug2("\tCapability regs: %p", hc->cap_regs);
	usb_log_debug2("\tOperational regs: %p", hc->op_regs);
	usb_log_debug2("\tRuntime regs: %p", hc->rt_regs);
	usb_log_debug2("\tDoorbell array base: %p", hc->db_arry);

	xhci_dump_cap_regs(hc->cap_regs);

	hc->ac64 = XHCI_REG_RD(hc->cap_regs, XHCI_CAP_AC64);
	hc->max_slots = XHCI_REG_RD(hc->cap_regs, XHCI_CAP_MAX_SLOTS);

	if ((err = hc_parse_ec(hc))) {
		pio_disable(hc->reg_base, RNGSZ(hc->mmio_range));
		return err;
	}

	return EOK;
}

int hc_init_memory(xhci_hc_t *hc, ddf_dev_t *device)
{
	int err;

	if (dma_buffer_alloc(&hc->dcbaa_dma, (1 + hc->max_slots) * sizeof(uint64_t)))
		return ENOMEM;
	hc->dcbaa = hc->dcbaa_dma.virt;

	if ((err = xhci_trb_ring_init(&hc->command_ring)))
		goto err_dcbaa;

	if ((err = xhci_event_ring_init(&hc->event_ring)))
		goto err_cmd_ring;

	if ((err = xhci_scratchpad_alloc(hc)))
		goto err_event_ring;

	if ((err = xhci_init_commands(hc)))
		goto err_scratch;

	if ((err = xhci_rh_init(&hc->rh, hc, device)))
		goto err_cmd;

	if ((err = xhci_bus_init(&hc->bus, hc)))
		goto err_rh;


	return EOK;

err_rh:
	xhci_rh_fini(&hc->rh);
err_cmd:
	xhci_fini_commands(hc);
err_scratch:
	xhci_scratchpad_free(hc);
err_event_ring:
	xhci_event_ring_fini(&hc->event_ring);
err_cmd_ring:
	xhci_trb_ring_fini(&hc->command_ring);
err_dcbaa:
	hc->dcbaa = NULL;
	dma_buffer_free(&hc->dcbaa_dma);
	return err;
}

/*
 * Pseudocode:
 *	ip = read(intr[0].iman)
 *	if (ip) {
 *		status = read(usbsts)
 *		assert status
 *		assert ip
 *		accept (passing status)
 *	}
 *	decline
 */
static const irq_cmd_t irq_commands[] = {
	{
		.cmd = CMD_PIO_READ_32,
		.dstarg = 3,
		.addr = NULL	/* intr[0].iman */
	},
	{
		.cmd = CMD_AND,
		.srcarg = 3,
		.dstarg = 4,
		.value = 0	/* host2xhci(32, 1) */
	},
	{
		.cmd = CMD_PREDICATE,
		.srcarg = 4,
		.value = 5
	},
	{
		.cmd = CMD_PIO_READ_32,
		.dstarg = 1,
		.addr = NULL	/* usbsts */
	},
	{
		.cmd = CMD_AND,
		.srcarg = 1,
		.dstarg = 2,
		.value = 0	/* host2xhci(32, XHCI_STATUS_ACK_MASK) */
	},
	{
		.cmd = CMD_PIO_WRITE_A_32,
		.srcarg = 2,
		.addr = NULL	/* usbsts */
	},
	{
		.cmd = CMD_PIO_WRITE_A_32,
		.srcarg = 3,
		.addr = NULL	/* intr[0].iman */
	},
	{
		.cmd = CMD_ACCEPT
	},
	{
		.cmd = CMD_DECLINE
	}
};


/**
 * Generates code to accept interrupts. The xHCI is designed primarily for
 * MSI/MSI-X, but we use PCI Interrupt Pin. In this mode, all the Interrupters
 * (except 0) are disabled.
 */
int hc_irq_code_gen(irq_code_t *code, xhci_hc_t *hc, const hw_res_list_parsed_t *hw_res)
{
	assert(code);
	assert(hw_res);

	if (hw_res->irqs.count != 1) {
		usb_log_info("Unexpected HW resources to enable interrupts.");
		return EINVAL;
	}

	code->ranges = malloc(sizeof(irq_pio_range_t));
	if (code->ranges == NULL)
		return ENOMEM;

	code->cmds = malloc(sizeof(irq_commands));
	if (code->cmds == NULL) {
		free(code->ranges);
		return ENOMEM;
	}

	code->rangecount = 1;
	code->ranges[0] = (irq_pio_range_t) {
	    .base = RNGABS(hc->mmio_range),
	    .size = RNGSZ(hc->mmio_range),
	};

	code->cmdcount = ARRAY_SIZE(irq_commands);
	memcpy(code->cmds, irq_commands, sizeof(irq_commands));

	void *intr0_iman = RNGABSPTR(hc->mmio_range) + XHCI_REG_RD(hc->cap_regs, XHCI_CAP_RTSOFF) + offsetof(xhci_rt_regs_t, ir[0]);
	void *usbsts = RNGABSPTR(hc->mmio_range) + XHCI_REG_RD(hc->cap_regs, XHCI_CAP_LENGTH) + offsetof(xhci_op_regs_t, usbsts);
	code->cmds[0].addr = intr0_iman;
	code->cmds[1].value = host2xhci(32, 1);
	code->cmds[3].addr = usbsts;
	code->cmds[4].value = host2xhci(32, XHCI_STATUS_ACK_MASK);
	code->cmds[5].addr = usbsts;
	code->cmds[6].addr = intr0_iman;

	return hw_res->irqs.irqs[0];
}

int hc_claim(xhci_hc_t *hc, ddf_dev_t *dev)
{
	/* No legacy support capability, the controller is solely for us */
	if (!hc->legsup)
		return EOK;

	/* Section 4.22.1 */
	/* TODO: Test this with USB3-aware BIOS */
	usb_log_debug2("LEGSUP: bios: %x, os: %x", hc->legsup->sem_bios, hc->legsup->sem_os);
	XHCI_REG_WR(hc->legsup, XHCI_LEGSUP_SEM_OS, 1);
	for (int i = 0; i <= (XHCI_LEGSUP_BIOS_TIMEOUT_US / XHCI_LEGSUP_POLLING_DELAY_1MS); i++) {
		usb_log_debug2("LEGSUP: elapsed: %i ms, bios: %x, os: %x", i,
			XHCI_REG_RD(hc->legsup, XHCI_LEGSUP_SEM_BIOS),
			XHCI_REG_RD(hc->legsup, XHCI_LEGSUP_SEM_OS));
		if (XHCI_REG_RD(hc->legsup, XHCI_LEGSUP_SEM_BIOS) == 0) {
			assert(XHCI_REG_RD(hc->legsup, XHCI_LEGSUP_SEM_OS) == 1);
			return EOK;
		}
		async_usleep(XHCI_LEGSUP_POLLING_DELAY_1MS);
	}
	usb_log_error("BIOS did not release XHCI legacy hold!\n");

	return ENOTSUP;
}

static int hc_reset(xhci_hc_t *hc)
{
	/* Stop the HC: set R/S to 0 */
	XHCI_REG_CLR(hc->op_regs, XHCI_OP_RS, 1);

	/* Wait 16 ms until the HC is halted */
	async_usleep(16000);
	assert(XHCI_REG_RD(hc->op_regs, XHCI_OP_HCH));

	/* Reset */
	XHCI_REG_SET(hc->op_regs, XHCI_OP_HCRST, 1);

	/* Wait until the reset is complete */
	while (XHCI_REG_RD(hc->op_regs, XHCI_OP_HCRST))
		async_usleep(1000);

	return EOK;
}

/**
 * Initialize the HC: section 4.2
 */
int hc_start(xhci_hc_t *hc, bool irq)
{
	int err;

	if ((err = hc_reset(hc)))
		return err;

	while (XHCI_REG_RD(hc->op_regs, XHCI_OP_CNR))
		async_usleep(1000);

	uint64_t dcbaaptr = hc->dcbaa_dma.phys;
	XHCI_REG_WR(hc->op_regs, XHCI_OP_DCBAAP_LO, LOWER32(dcbaaptr));
	XHCI_REG_WR(hc->op_regs, XHCI_OP_DCBAAP_HI, UPPER32(dcbaaptr));
	XHCI_REG_WR(hc->op_regs, XHCI_OP_MAX_SLOTS_EN, 0);

	uint64_t crptr = xhci_trb_ring_get_dequeue_ptr(&hc->command_ring);
	XHCI_REG_WR(hc->op_regs, XHCI_OP_CRCR_LO, LOWER32(crptr) >> 6);
	XHCI_REG_WR(hc->op_regs, XHCI_OP_CRCR_HI, UPPER32(crptr));

	xhci_interrupter_regs_t *intr0 = &hc->rt_regs->ir[0];
	XHCI_REG_WR(intr0, XHCI_INTR_ERSTSZ, hc->event_ring.segment_count);
	uint64_t erdp = hc->event_ring.dequeue_ptr;
	XHCI_REG_WR(intr0, XHCI_INTR_ERDP_LO, LOWER32(erdp));
	XHCI_REG_WR(intr0, XHCI_INTR_ERDP_HI, UPPER32(erdp));
	uint64_t erstptr = hc->event_ring.erst.phys;
	XHCI_REG_WR(intr0, XHCI_INTR_ERSTBA_LO, LOWER32(erstptr));
	XHCI_REG_WR(intr0, XHCI_INTR_ERSTBA_HI, UPPER32(erstptr));

	if (irq) {
		XHCI_REG_SET(intr0, XHCI_INTR_IE, 1);
		XHCI_REG_SET(hc->op_regs, XHCI_OP_INTE, 1);
	}

	XHCI_REG_SET(hc->op_regs, XHCI_OP_RS, 1);

	/* The reset changed status of all ports, and SW originated reason does
	 * not cause an interrupt.
	 */
	xhci_rh_handle_port_change(&hc->rh);

	return EOK;
}

/**
 * Used only when polling. Shall supplement the irq_commands.
 */
int hc_status(xhci_hc_t *hc, uint32_t *status)
{
	int ip = XHCI_REG_RD(hc->rt_regs->ir, XHCI_INTR_IP);
	if (ip) {
		*status = XHCI_REG_RD(hc->op_regs, XHCI_OP_STATUS);
		XHCI_REG_WR(hc->op_regs, XHCI_OP_STATUS, *status & XHCI_STATUS_ACK_MASK);
		XHCI_REG_WR(hc->rt_regs->ir, XHCI_INTR_IP, 1);

		/* interrupt handler expects status from irq_commands, which is
		 * in xhci order. */
		*status = host2xhci(32, *status);
	}

	usb_log_debug2("HC(%p): Polled status: %x", hc, *status);
	return EOK;
}

int hc_schedule(xhci_hc_t *hc, usb_transfer_batch_t *batch)
{
	assert(batch);
	assert(batch->ep);

	if (!batch->target.address) {
		usb_log_error("Attempted to schedule transfer to address 0.");
		return EINVAL;
	}

	return xhci_transfer_schedule(hc, batch);
}

typedef int (*event_handler) (xhci_hc_t *, xhci_trb_t *trb);

static event_handler event_handlers [] = {
	[XHCI_TRB_TYPE_COMMAND_COMPLETION_EVENT] = &xhci_handle_command_completion,
	[XHCI_TRB_TYPE_PORT_STATUS_CHANGE_EVENT] = &xhci_rh_handle_port_status_change_event,
	[XHCI_TRB_TYPE_TRANSFER_EVENT] = &xhci_handle_transfer_event,
};

static int hc_handle_event(xhci_hc_t *hc, xhci_trb_t *trb, xhci_interrupter_regs_t *intr)
{
	unsigned type = TRB_TYPE(*trb);
	if (type >= ARRAY_SIZE(event_handlers) || !event_handlers[type])
		return ENOTSUP;

	return event_handlers[type](hc, trb);
}

static void hc_run_event_ring(xhci_hc_t *hc, xhci_event_ring_t *event_ring, xhci_interrupter_regs_t *intr)
{
	int err;
	ssize_t size = 16;
	xhci_trb_t *queue = malloc(sizeof(xhci_trb_t) * size);
	if (!queue) {
		usb_log_error("Not enough memory to run the event ring.");
		return;
	}

	xhci_trb_t *head = queue;

	while ((err = xhci_event_ring_dequeue(event_ring, head)) != ENOENT) {
		if (err != EOK) {
			usb_log_warning("Error while accessing event ring: %s", str_error(err));
			break;
		}

		usb_log_debug2("Dequeued trb from event ring: %s", xhci_trb_str_type(TRB_TYPE(*head)));
		head++;

		/* Expand the array if needed. */
		if (head - queue >= size) {
			size *= 2;
			xhci_trb_t *new_queue = realloc(queue, size);
			if (new_queue == NULL)
				break; /* Will process only those TRBs we have memory for. */

			head = new_queue + (head - queue);
		}
	}

	/* Update the ERDP to make room in the ring. */
	usb_log_debug2("Copying from ring finished, updating ERDP.");
	uint64_t erdp = hc->event_ring.dequeue_ptr;
	XHCI_REG_WR(intr, XHCI_INTR_ERDP_LO, LOWER32(erdp));
	XHCI_REG_WR(intr, XHCI_INTR_ERDP_HI, UPPER32(erdp));
	XHCI_REG_SET(intr, XHCI_INTR_ERDP_EHB, 1);

	/* Handle all of the collected events if possible. */
	if (head == queue)
		usb_log_warning("No events to be handled!");

	for (xhci_trb_t *tail = queue; tail != head; tail++) {
		if ((err = hc_handle_event(hc, tail, intr)) != EOK) {
			usb_log_error("Failed to handle event: %s", str_error(err));
		}
	}

	free(queue);
	usb_log_debug2("Event ring run finished.");
}

void hc_interrupt(xhci_hc_t *hc, uint32_t status)
{
	status = xhci2host(32, status);

	if (status & XHCI_REG_MASK(XHCI_OP_PCD)) {
		usb_log_debug2("Root hub interrupt.");
		xhci_rh_handle_port_change(&hc->rh);
		status &= ~XHCI_REG_MASK(XHCI_OP_PCD);
	}

	if (status & XHCI_REG_MASK(XHCI_OP_HSE)) {
		usb_log_error("Host controller error occured. Bad things gonna happen...");
		status &= ~XHCI_REG_MASK(XHCI_OP_HSE);
	}

	if (status & XHCI_REG_MASK(XHCI_OP_EINT)) {
		usb_log_debug2("Event interrupt, running the event ring.");
		hc_run_event_ring(hc, &hc->event_ring, &hc->rt_regs->ir[0]);
		status &= ~XHCI_REG_MASK(XHCI_OP_EINT);
	}

	if (status & XHCI_REG_MASK(XHCI_OP_SRE)) {
		usb_log_error("Save/Restore error occured. WTF, S/R mechanism not implemented!");
		status &= ~XHCI_REG_MASK(XHCI_OP_SRE);
	}

	if (status) {
		usb_log_error("Non-zero status after interrupt handling (%08x) - missing something?", status);
	}
}

static void hc_dcbaa_fini(xhci_hc_t *hc)
{
	xhci_scratchpad_free(hc);
	dma_buffer_free(&hc->dcbaa_dma);
}

void hc_fini(xhci_hc_t *hc)
{
	xhci_bus_fini(&hc->bus);
	xhci_trb_ring_fini(&hc->command_ring);
	xhci_event_ring_fini(&hc->event_ring);
	hc_dcbaa_fini(hc);
	xhci_fini_commands(hc);
	xhci_rh_fini(&hc->rh);
	pio_disable(hc->reg_base, RNGSZ(hc->mmio_range));
	usb_log_info("HC(%p): Finalized.", hc);
}

int hc_ring_doorbell(xhci_hc_t *hc, unsigned doorbell, unsigned target)
{
	assert(hc);
	uint32_t v = host2xhci(32, target & BIT_RRANGE(uint32_t, 7));
	pio_write_32(&hc->db_arry[doorbell], v);
	usb_log_debug2("Ringing doorbell %d (target: %d)", doorbell, target);
	return EOK;
}

int hc_enable_slot(xhci_hc_t *hc, uint32_t *slot_id)
{
	assert(hc);

	int err;
	xhci_cmd_t cmd;
	xhci_cmd_init(&cmd, XHCI_CMD_ENABLE_SLOT);

	if ((err = xhci_cmd_sync(hc, &cmd))) {
		goto end;
	}

	if (slot_id) {
		*slot_id = cmd.slot_id;
	}

end:
	xhci_cmd_fini(&cmd);
	return err;
}

int hc_disable_slot(xhci_hc_t *hc, xhci_device_t *dev)
{
	int err;
	assert(hc);

	if ((err = xhci_cmd_sync_inline(hc, DISABLE_SLOT, .slot_id = dev->slot_id))) {
		return err;
	}

	/* Free the device context. */
	hc->dcbaa[dev->slot_id] = 0;
	dma_buffer_free(&dev->dev_ctx);

	/* Mark the slot as invalid. */
	dev->slot_id = 0;

	return EOK;
}

static int create_configure_ep_input_ctx(dma_buffer_t *dma_buf)
{
	const int err = dma_buffer_alloc(dma_buf, sizeof(xhci_input_ctx_t));
	if (err)
		return err;

	xhci_input_ctx_t *ictx = dma_buf->virt;
	memset(ictx, 0, sizeof(xhci_input_ctx_t));

	// Quoting sec. 4.6.5 and 4.6.6: A1, D0, D1 are down (already zeroed), A0 is up.
	XHCI_INPUT_CTRL_CTX_ADD_SET(ictx->ctrl_ctx, 0);

	return EOK;
}

// TODO: This currently assumes the device is attached to rh directly
//	-> calculate route string
int hc_address_device(xhci_hc_t *hc, xhci_device_t *dev, xhci_endpoint_t *ep0)
{
	int err = ENOMEM;

	/* Although we have the precise PSIV value on devices of tier 1,
	 * we have to rely on reverse mapping on others. */
	if (!hc->speed_to_psiv[dev->base.speed]) {
		usb_log_error("Device reported an USB speed that cannot be mapped to HC port speed.");
		return EINVAL;
	}

	/* Setup and register device context */
	if (dma_buffer_alloc(&dev->dev_ctx, sizeof(xhci_device_ctx_t)))
		goto err;
	memset(dev->dev_ctx.virt, 0, sizeof(xhci_device_ctx_t));

	hc->dcbaa[dev->slot_id] = host2xhci(64, dev->dev_ctx.phys);

	/* Issue configure endpoint command (sec 4.3.5). */
	dma_buffer_t ictx_dma_buf;
	if ((err = create_configure_ep_input_ctx(&ictx_dma_buf))) {
		goto err_dev_ctx;
	}
	xhci_input_ctx_t *ictx = ictx_dma_buf.virt;

	/* Initialize slot_ctx according to section 4.3.3 point 3. */
	XHCI_SLOT_ROOT_HUB_PORT_SET(ictx->slot_ctx, dev->rh_port);
	XHCI_SLOT_CTX_ENTRIES_SET(ictx->slot_ctx, 1);
	XHCI_SLOT_ROUTE_STRING_SET(ictx->slot_ctx, dev->route_str);
	XHCI_SLOT_SPEED_SET(ictx->slot_ctx, hc->speed_to_psiv[dev->base.speed]);

	/* In a very specific case, we have to set also these. But before that,
	 * we need to refactor how TT is handled in libusbhost. */
	XHCI_SLOT_TT_HUB_SLOT_ID_SET(ictx->slot_ctx, 0);
	XHCI_SLOT_TT_HUB_PORT_SET(ictx->slot_ctx, 0);
	XHCI_SLOT_MTT_SET(ictx->slot_ctx, 0);

	/* Copy endpoint 0 context and set A1 flag. */
	XHCI_INPUT_CTRL_CTX_ADD_SET(ictx->ctrl_ctx, 1);
	xhci_setup_endpoint_context(ep0, &ictx->endpoint_ctx[0]);

	/* Issue Address Device command. */
	if ((err = xhci_cmd_sync_inline(hc, ADDRESS_DEVICE, .slot_id = dev->slot_id, .input_ctx = ictx_dma_buf))) {
		goto err_dev_ctx;
	}

	xhci_device_ctx_t *dev_ctx = dev->dev_ctx.virt;
	dev->base.address = XHCI_SLOT_DEVICE_ADDRESS(dev_ctx->slot_ctx);
	usb_log_debug2("Obtained USB address: %d.\n", dev->base.address);

	/* From now on, the device is officially online, yay! */
	fibril_mutex_lock(&dev->base.guard);
	dev->online = true;
	fibril_mutex_unlock(&dev->base.guard);

	return EOK;

err_dev_ctx:
	hc->dcbaa[dev->slot_id] = 0;
	dma_buffer_free(&dev->dev_ctx);
err:
	return err;
}

int hc_configure_device(xhci_hc_t *hc, uint32_t slot_id)
{
	/* Issue configure endpoint command (sec 4.3.5). */
	dma_buffer_t ictx_dma_buf;
	const int err = create_configure_ep_input_ctx(&ictx_dma_buf);
	if (err)
		return err;

	// TODO: Set slot context and other flags. (probably forgot a lot of 'em)

	return xhci_cmd_sync_inline(hc, CONFIGURE_ENDPOINT, .slot_id = slot_id, .input_ctx = ictx_dma_buf);
}

int hc_deconfigure_device(xhci_hc_t *hc, uint32_t slot_id)
{
	/* Issue configure endpoint command (sec 4.3.5) with the DC flag. */
	return xhci_cmd_sync_inline(hc, CONFIGURE_ENDPOINT, .slot_id = slot_id, .deconfigure = true);
}

int hc_add_endpoint(xhci_hc_t *hc, uint32_t slot_id, uint8_t ep_idx, xhci_ep_ctx_t *ep_ctx)
{
	/* Issue configure endpoint command (sec 4.3.5). */
	dma_buffer_t ictx_dma_buf;
	const int err = create_configure_ep_input_ctx(&ictx_dma_buf);
	if (err)
		return err;

	xhci_input_ctx_t *ictx = ictx_dma_buf.virt;
	XHCI_INPUT_CTRL_CTX_ADD_SET(ictx->ctrl_ctx, ep_idx + 1); /* Preceded by slot ctx */
	memcpy(&ictx->endpoint_ctx[ep_idx], ep_ctx, sizeof(xhci_ep_ctx_t));
	// TODO: Set slot context and other flags. (probably forgot a lot of 'em)

	return xhci_cmd_sync_inline(hc, CONFIGURE_ENDPOINT, .slot_id = slot_id, .input_ctx = ictx_dma_buf);
}

int hc_drop_endpoint(xhci_hc_t *hc, uint32_t slot_id, uint8_t ep_idx)
{
	/* Issue configure endpoint command (sec 4.3.5). */
	dma_buffer_t ictx_dma_buf;
	const int err = create_configure_ep_input_ctx(&ictx_dma_buf);
	if (err)
		return err;

	xhci_input_ctx_t *ictx = ictx_dma_buf.virt;
	XHCI_INPUT_CTRL_CTX_DROP_SET(ictx->ctrl_ctx, ep_idx + 1); /* Preceded by slot ctx */
	// TODO: Set slot context and other flags. (probably forgot a lot of 'em)

	return xhci_cmd_sync_inline(hc, CONFIGURE_ENDPOINT, .slot_id = slot_id, .input_ctx = ictx_dma_buf);
}

/**
 * @}
 */
