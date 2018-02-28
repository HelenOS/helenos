/*
 * Copyright (c) 2018 Ondrej Hlavaty, Petr Manek, Jaroslav Jindrak, Jan Hrach, Michal Staruch
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

static const xhci_port_speed_t default_psiv_to_port_speed [] = {
	[1] = PORT_SPEED(FULL, 2, 2, 12),
	[2] = PORT_SPEED(LOW, 2, 1, 1500),
	[3] = PORT_SPEED(HIGH, 2, 2, 480),
	[4] = PORT_SPEED(SUPER, 3, 3, 5),
};

static const unsigned usb_speed_to_psiv [] = {
	[USB_SPEED_FULL] = 1,
	[USB_SPEED_LOW] = 2,
	[USB_SPEED_HIGH] = 3,
	[USB_SPEED_SUPER] = 4,
};

/**
 * Walk the list of extended capabilities.
 *
 * The most interesting thing hidden in extended capabilities is the mapping of
 * ports to protocol versions and speeds.
 */
static errno_t hc_parse_ec(xhci_hc_t *hc)
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

			unsigned offset = XHCI_REG_RD(ec, XHCI_EC_SP_CP_OFF);
			unsigned count = XHCI_REG_RD(ec, XHCI_EC_SP_CP_COUNT);
			xhci_rh_set_ports_protocol(&hc->rh, offset, count, major);

			// "Implied" speed
			if (psic == 0) {
				assert(minor == 0);

				if (major == 2) {
					speeds[1] = default_psiv_to_port_speed[1];
					speeds[2] = default_psiv_to_port_speed[2];
					speeds[3] = default_psiv_to_port_speed[3];
				} else if (major == 3) {
					speeds[4] = default_psiv_to_port_speed[4];
				} else {
					return EINVAL;
				}

				usb_log_debug("Implied speed of USB %u.0 set up.", major);
			} else {
				for (unsigned i = 0; i < psic; i++) {
					xhci_psi_t *psi = xhci_extcap_psi(ec, i);
					unsigned sim = XHCI_REG_RD(psi, XHCI_PSI_PSIM);
					unsigned psiv = XHCI_REG_RD(psi, XHCI_PSI_PSIV);
					unsigned psie = XHCI_REG_RD(psi, XHCI_PSI_PSIE);
					unsigned psim = XHCI_REG_RD(psi, XHCI_PSI_PSIM);
					uint64_t bps = PSI_TO_BPS(psie, psim);

					/*
					 * Speed is not implied, but using one of default PSIV. This
					 * is not clearly stated in xHCI spec. There is a clear
					 * intention to allow xHCI to specify its own speed
					 * parameters, but throughout the document, they used fixed
					 * values for e.g. High-speed (3), without stating the
					 * controller shall have implied default speeds - and for
					 * instance Intel controllers do not. So let's check if the
					 * values match and if so, accept the implied USB speed too.
					 *
					 * The main reason we need this is the usb_speed to have
					 * mapping also for devices connected to hubs.
					 */
					if (psiv < ARRAY_SIZE(default_psiv_to_port_speed)
					   && default_psiv_to_port_speed[psiv].major == major
					   && default_psiv_to_port_speed[psiv].minor == minor
					   && default_psiv_to_port_speed[psiv].rx_bps == bps
					   && default_psiv_to_port_speed[psiv].tx_bps == bps) {
						speeds[psiv] = default_psiv_to_port_speed[psiv];
						usb_log_debug("Assumed default %s speed of USB %u.",
							usb_str_speed(speeds[psiv].usb_speed), major);
						continue;
					}

					// Custom speed
					speeds[psiv].major = major;
					speeds[psiv].minor = minor;
					str_ncpy(speeds[psiv].name, 4, name.str, 4);
					speeds[psiv].usb_speed = USB_SPEED_MAX;

					if (sim == XHCI_PSI_PLT_SYMM || sim == XHCI_PSI_PLT_RX)
						speeds[psiv].rx_bps = bps;
					if (sim == XHCI_PSI_PLT_SYMM || sim == XHCI_PSI_PLT_TX) {
						speeds[psiv].tx_bps = bps;
						usb_log_debug("Speed %u set up for bps %" PRIu64
							" / %" PRIu64 ".", psiv, speeds[psiv].rx_bps,
							speeds[psiv].tx_bps);
					}
				}
			}
		}
	}
	return EOK;
}

/**
 * Initialize MMIO spaces of xHC.
 */
errno_t hc_init_mmio(xhci_hc_t *hc, const hw_res_list_parsed_t *hw_res)
{
	errno_t err;

	if (hw_res->mem_ranges.count != 1) {
		usb_log_error("Unexpected MMIO area, bailing out.");
		return EINVAL;
	}

	hc->mmio_range = hw_res->mem_ranges.ranges[0];

	usb_log_debug("MMIO area at %p (size %zu), IRQ %d.",
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

	usb_log_debug("Initialized MMIO reg areas:");
	usb_log_debug("\tCapability regs: %p", hc->cap_regs);
	usb_log_debug("\tOperational regs: %p", hc->op_regs);
	usb_log_debug("\tRuntime regs: %p", hc->rt_regs);
	usb_log_debug("\tDoorbell array base: %p", hc->db_arry);

	xhci_dump_cap_regs(hc->cap_regs);

	hc->ac64 = XHCI_REG_RD(hc->cap_regs, XHCI_CAP_AC64);
	hc->csz = XHCI_REG_RD(hc->cap_regs, XHCI_CAP_CSZ);
	hc->max_slots = XHCI_REG_RD(hc->cap_regs, XHCI_CAP_MAX_SLOTS);

	struct timeval tv;
	getuptime(&tv);
	hc->wrap_time = tv.tv_sec * 1000000 + tv.tv_usec;
	hc->wrap_count = 0;

	unsigned ist = XHCI_REG_RD(hc->cap_regs, XHCI_CAP_IST);
	hc->ist = (ist & 0x10 >> 1) * (ist & 0xf);

	if ((err = xhci_rh_init(&hc->rh, hc)))
		goto err_pio;

	if ((err = hc_parse_ec(hc)))
		goto err_rh;

	return EOK;

err_rh:
	xhci_rh_fini(&hc->rh);
err_pio:
	pio_disable(hc->reg_base, RNGSZ(hc->mmio_range));
	return err;
}

static int event_worker(void *arg);

/**
 * Initialize structures kept in allocated memory.
 */
errno_t hc_init_memory(xhci_hc_t *hc, ddf_dev_t *device)
{
	errno_t err = ENOMEM;

	if (dma_buffer_alloc(&hc->dcbaa_dma, (1 + hc->max_slots) * sizeof(uint64_t)))
		return ENOMEM;
	hc->dcbaa = hc->dcbaa_dma.virt;

	hc->event_worker = joinable_fibril_create(&event_worker, hc);
	if (!hc->event_worker)
		goto err_dcbaa;

	if ((err = xhci_event_ring_init(&hc->event_ring, 1)))
		goto err_worker;

	if ((err = xhci_scratchpad_alloc(hc)))
		goto err_event_ring;

	if ((err = xhci_init_commands(hc)))
		goto err_scratch;

	if ((err = xhci_bus_init(&hc->bus, hc)))
		goto err_cmd;

	xhci_sw_ring_init(&hc->sw_ring, PAGE_SIZE / sizeof(xhci_trb_t));

	return EOK;

err_cmd:
	xhci_fini_commands(hc);
err_scratch:
	xhci_scratchpad_free(hc);
err_event_ring:
	xhci_event_ring_fini(&hc->event_ring);
err_worker:
	joinable_fibril_destroy(hc->event_worker);
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
errno_t hc_irq_code_gen(irq_code_t *code, xhci_hc_t *hc, const hw_res_list_parsed_t *hw_res, int *irq)
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

	void *intr0_iman = RNGABSPTR(hc->mmio_range)
	    + XHCI_REG_RD(hc->cap_regs, XHCI_CAP_RTSOFF)
	    + offsetof(xhci_rt_regs_t, ir[0]);
	void *usbsts = RNGABSPTR(hc->mmio_range)
	    + XHCI_REG_RD(hc->cap_regs, XHCI_CAP_LENGTH)
	    + offsetof(xhci_op_regs_t, usbsts);

	code->cmds[0].addr = intr0_iman;
	code->cmds[1].value = host2xhci(32, 1);
	code->cmds[3].addr = usbsts;
	code->cmds[4].value = host2xhci(32, XHCI_STATUS_ACK_MASK);
	code->cmds[5].addr = usbsts;
	code->cmds[6].addr = intr0_iman;

	*irq = hw_res->irqs.irqs[0];
	return EOK;
}

/**
 * Claim xHC from BIOS. Implements handoff as per Section 4.22.1 of xHCI spec.
 */
errno_t hc_claim(xhci_hc_t *hc, ddf_dev_t *dev)
{
	/* No legacy support capability, the controller is solely for us */
	if (!hc->legsup)
		return EOK;

	if (xhci_reg_wait(&hc->op_regs->usbsts, XHCI_REG_MASK(XHCI_OP_CNR), 0))
		return ETIMEOUT;

	usb_log_debug("LEGSUP: bios: %x, os: %x", hc->legsup->sem_bios, hc->legsup->sem_os);
	XHCI_REG_SET(hc->legsup, XHCI_LEGSUP_SEM_OS, 1);
	for (int i = 0; i <= (XHCI_LEGSUP_BIOS_TIMEOUT_US / XHCI_LEGSUP_POLLING_DELAY_1MS); i++) {
		usb_log_debug("LEGSUP: elapsed: %i ms, bios: %x, os: %x", i,
			XHCI_REG_RD(hc->legsup, XHCI_LEGSUP_SEM_BIOS),
			XHCI_REG_RD(hc->legsup, XHCI_LEGSUP_SEM_OS));
		if (XHCI_REG_RD(hc->legsup, XHCI_LEGSUP_SEM_BIOS) == 0) {
			return XHCI_REG_RD(hc->legsup, XHCI_LEGSUP_SEM_OS) == 1 ? EOK : EIO;
		}
		async_usleep(XHCI_LEGSUP_POLLING_DELAY_1MS);
	}
	usb_log_error("BIOS did not release XHCI legacy hold!");

	return ENOTSUP;
}

/**
 * Ask the xHC to reset its state. Implements sequence
 */
static errno_t hc_reset(xhci_hc_t *hc)
{
	if (xhci_reg_wait(&hc->op_regs->usbsts, XHCI_REG_MASK(XHCI_OP_CNR), 0))
		return ETIMEOUT;

	/* Stop the HC: set R/S to 0 */
	XHCI_REG_CLR(hc->op_regs, XHCI_OP_RS, 1);

	/* Wait until the HC is halted - it shall take at most 16 ms */
	if (xhci_reg_wait(&hc->op_regs->usbsts, XHCI_REG_MASK(XHCI_OP_HCH),
	    XHCI_REG_MASK(XHCI_OP_HCH)))
		return ETIMEOUT;

	/* Reset */
	XHCI_REG_SET(hc->op_regs, XHCI_OP_HCRST, 1);

	/* Wait until the reset is complete */
	if (xhci_reg_wait(&hc->op_regs->usbcmd, XHCI_REG_MASK(XHCI_OP_HCRST), 0))
		return ETIMEOUT;

	return EOK;
}

/**
 * Initialize the HC: section 4.2
 */
errno_t hc_start(xhci_hc_t *hc)
{
	errno_t err;

	if ((err = hc_reset(hc)))
		return err;

	if (xhci_reg_wait(&hc->op_regs->usbsts, XHCI_REG_MASK(XHCI_OP_CNR), 0))
		return ETIMEOUT;

	uintptr_t dcbaa_phys = dma_buffer_phys_base(&hc->dcbaa_dma);
	XHCI_REG_WR(hc->op_regs, XHCI_OP_DCBAAP, dcbaa_phys);
	XHCI_REG_WR(hc->op_regs, XHCI_OP_MAX_SLOTS_EN, hc->max_slots);

	uintptr_t crcr;
	xhci_trb_ring_reset_dequeue_state(&hc->cr.trb_ring, &crcr);
	XHCI_REG_WR(hc->op_regs, XHCI_OP_CRCR, crcr);

	XHCI_REG_SET(hc->op_regs, XHCI_OP_EWE, 1);

	xhci_event_ring_reset(&hc->event_ring);

	xhci_interrupter_regs_t *intr0 = &hc->rt_regs->ir[0];
	XHCI_REG_WR(intr0, XHCI_INTR_ERSTSZ, hc->event_ring.segment_count);
	XHCI_REG_WR(intr0, XHCI_INTR_ERDP, hc->event_ring.dequeue_ptr);

	const uintptr_t erstba_phys = dma_buffer_phys_base(&hc->event_ring.erst);
	XHCI_REG_WR(intr0, XHCI_INTR_ERSTBA, erstba_phys);

	if (hc->base.irq_cap > 0) {
		XHCI_REG_SET(intr0, XHCI_INTR_IE, 1);
		XHCI_REG_SET(hc->op_regs, XHCI_OP_INTE, 1);
	}

	XHCI_REG_SET(hc->op_regs, XHCI_OP_HSEE, 1);

	xhci_sw_ring_restart(&hc->sw_ring);
	joinable_fibril_start(hc->event_worker);

	xhci_start_command_ring(hc);

	XHCI_REG_SET(hc->op_regs, XHCI_OP_RS, 1);

	/* RH needs to access port states on startup */
	xhci_rh_start(&hc->rh);

	return EOK;
}

static void hc_stop(xhci_hc_t *hc)
{
	/* Stop the HC in hardware. */
	XHCI_REG_CLR(hc->op_regs, XHCI_OP_RS, 1);

	/*
	 * Wait until the HC is halted - it shall take at most 16 ms.
	 * Note that we ignore the return value here.
	 */
	xhci_reg_wait(&hc->op_regs->usbsts, XHCI_REG_MASK(XHCI_OP_HCH),
	    XHCI_REG_MASK(XHCI_OP_HCH));

	/* Make sure commands will not block other fibrils. */
	xhci_nuke_command_ring(hc);

	/* Stop the event worker fibril to restart it */
	xhci_sw_ring_stop(&hc->sw_ring);
	joinable_fibril_join(hc->event_worker);

	/* Then, disconnect all roothub devices, which shall trigger
	 * disconnection of everything */
	xhci_rh_stop(&hc->rh);
}

static void hc_reinitialize(xhci_hc_t *hc)
{
	/* Stop everything. */
	hc_stop(hc);

	usb_log_info("HC stopped. Starting again...");

	/* The worker fibrils need to be started again */
	joinable_fibril_recreate(hc->event_worker);
	joinable_fibril_recreate(hc->rh.event_worker);

	/* Now, the HC shall be stopped and software shall be clean. */
	hc_start(hc);
}

static bool hc_is_broken(xhci_hc_t *hc)
{
	const uint32_t usbcmd = XHCI_REG_RD_FIELD(&hc->op_regs->usbcmd, 32);
	const uint32_t usbsts = XHCI_REG_RD_FIELD(&hc->op_regs->usbsts, 32);

	return !(usbcmd & XHCI_REG_MASK(XHCI_OP_RS))
	    ||  (usbsts & XHCI_REG_MASK(XHCI_OP_HCE))
	    ||  (usbsts & XHCI_REG_MASK(XHCI_OP_HSE));
}

/**
 * Used only when polling. Shall supplement the irq_commands.
 */
errno_t hc_status(bus_t *bus, uint32_t *status)
{
	xhci_hc_t *hc = bus_to_hc(bus);
	int ip = XHCI_REG_RD(hc->rt_regs->ir, XHCI_INTR_IP);
	if (ip) {
		*status = XHCI_REG_RD(hc->op_regs, XHCI_OP_STATUS);
		XHCI_REG_WR(hc->op_regs, XHCI_OP_STATUS, *status & XHCI_STATUS_ACK_MASK);
		XHCI_REG_WR(hc->rt_regs->ir, XHCI_INTR_IP, 1);

		/* interrupt handler expects status from irq_commands, which is
		 * in xhci order. */
		*status = host2xhci(32, *status);
	}

	usb_log_debug("Polled status: %x", *status);
	return EOK;
}

static errno_t xhci_handle_mfindex_wrap_event(xhci_hc_t *hc, xhci_trb_t *trb)
{
	struct timeval tv;
	getuptime(&tv);
	usb_log_debug("Microframe index wrapped (@%lu.%li, %"PRIu64" total).",
	    tv.tv_sec, tv.tv_usec, hc->wrap_count);
	hc->wrap_time = ((uint64_t) tv.tv_sec) * 1000000 + ((uint64_t) tv.tv_usec);
	++hc->wrap_count;
	return EOK;
}

typedef errno_t (*event_handler) (xhci_hc_t *, xhci_trb_t *trb);

/**
 * These events are handled by separate event handling fibril.
 */
static event_handler event_handlers [] = {
	[XHCI_TRB_TYPE_TRANSFER_EVENT] = &xhci_handle_transfer_event,
};

/**
 * These events are handled directly in the interrupt handler, thus they must
 * not block waiting for another interrupt.
 */
static event_handler event_handlers_fast [] = {
	[XHCI_TRB_TYPE_COMMAND_COMPLETION_EVENT] = &xhci_handle_command_completion,
	[XHCI_TRB_TYPE_MFINDEX_WRAP_EVENT] = &xhci_handle_mfindex_wrap_event,
};

static errno_t hc_handle_event(xhci_hc_t *hc, xhci_trb_t *trb)
{
	const unsigned type = TRB_TYPE(*trb);

	if (type <= ARRAY_SIZE(event_handlers_fast) && event_handlers_fast[type])
		return event_handlers_fast[type](hc, trb);

	if (type <= ARRAY_SIZE(event_handlers) && event_handlers[type])
		return xhci_sw_ring_enqueue(&hc->sw_ring, trb);

	if (type == XHCI_TRB_TYPE_PORT_STATUS_CHANGE_EVENT)
		return xhci_sw_ring_enqueue(&hc->rh.event_ring, trb);

	return ENOTSUP;
}

static int event_worker(void *arg)
{
	errno_t err;
	xhci_trb_t trb;
	xhci_hc_t * const hc = arg;
	assert(hc);

	while (xhci_sw_ring_dequeue(&hc->sw_ring, &trb) != EINTR) {
		const unsigned type = TRB_TYPE(trb);

		if ((err = event_handlers[type](hc, &trb)))
			usb_log_error("Failed to handle event: %s", str_error(err));
	}

	return 0;
}

/**
 * Dequeue from event ring and handle dequeued events.
 *
 * As there can be events, that blocks on waiting for subsequent events,
 * we solve this problem by deferring some types of events to separate fibrils.
 */
static void hc_run_event_ring(xhci_hc_t *hc, xhci_event_ring_t *event_ring,
	xhci_interrupter_regs_t *intr)
{
	errno_t err;

	xhci_trb_t trb;
	hc->event_handler = fibril_get_id();

	while ((err = xhci_event_ring_dequeue(event_ring, &trb)) != ENOENT) {
		if ((err = hc_handle_event(hc, &trb)) != EOK) {
			usb_log_error("Failed to handle event in interrupt: %s", str_error(err));
		}

		XHCI_REG_WR(intr, XHCI_INTR_ERDP, hc->event_ring.dequeue_ptr);
	}

	hc->event_handler = 0;

	uint64_t erdp = hc->event_ring.dequeue_ptr;
	erdp |= XHCI_REG_MASK(XHCI_INTR_ERDP_EHB);
	XHCI_REG_WR(intr, XHCI_INTR_ERDP, erdp);

	usb_log_debug2("Event ring run finished.");
}

/**
 * Handle an interrupt request from xHC. Resolve all situations that trigger an
 * interrupt separately.
 *
 * Note that all RW1C bits in USBSTS register are cleared at the time of
 * handling the interrupt in irq_code. This method is the top-half.
 *
 * @param status contents of USBSTS register at the time of the interrupt.
 */
void hc_interrupt(bus_t *bus, uint32_t status)
{
	xhci_hc_t *hc = bus_to_hc(bus);
	status = xhci2host(32, status);

	if (status & XHCI_REG_MASK(XHCI_OP_HSE)) {
		usb_log_error("Host system error occured. Aren't we supposed to be dead already?");
		return;
	}

	if (status & XHCI_REG_MASK(XHCI_OP_HCE)) {
		usb_log_error("Host controller error occured. Reinitializing...");
		hc_reinitialize(hc);
		return;
	}

	if (status & XHCI_REG_MASK(XHCI_OP_EINT)) {
		usb_log_debug2("Event interrupt, running the event ring.");
		hc_run_event_ring(hc, &hc->event_ring, &hc->rt_regs->ir[0]);
		status &= ~XHCI_REG_MASK(XHCI_OP_EINT);
	}

	if (status & XHCI_REG_MASK(XHCI_OP_SRE)) {
		usb_log_error("Save/Restore error occured. WTF, "
		    "S/R mechanism not implemented!");
		status &= ~XHCI_REG_MASK(XHCI_OP_SRE);
	}

	/* According to Note on p. 302, we may safely ignore the PCD bit. */
	status &= ~XHCI_REG_MASK(XHCI_OP_PCD);

	if (status) {
		usb_log_error("Non-zero status after interrupt handling (%08x) "
			" - missing something?", status);
	}
}

/**
 * Tear down all in-memory structures.
 */
void hc_fini(xhci_hc_t *hc)
{
	hc_stop(hc);

	xhci_sw_ring_fini(&hc->sw_ring);
	joinable_fibril_destroy(hc->event_worker);
	xhci_bus_fini(&hc->bus);
	xhci_event_ring_fini(&hc->event_ring);
	xhci_scratchpad_free(hc);
	dma_buffer_free(&hc->dcbaa_dma);
	xhci_fini_commands(hc);
	xhci_rh_fini(&hc->rh);
	pio_disable(hc->reg_base, RNGSZ(hc->mmio_range));
	usb_log_info("Finalized.");
}

unsigned hc_speed_to_psiv(usb_speed_t speed)
{
	assert(speed < ARRAY_SIZE(usb_speed_to_psiv));
	return usb_speed_to_psiv[speed];
}

/**
 * Ring a xHC Doorbell. Implements section 4.7.
 */
void hc_ring_doorbell(xhci_hc_t *hc, unsigned doorbell, unsigned target)
{
	assert(hc);
	uint32_t v = host2xhci(32, target & BIT_RRANGE(uint32_t, 7));
	pio_write_32(&hc->db_arry[doorbell], v);
	usb_log_debug2("Ringing doorbell %d (target: %d)", doorbell, target);
}

/**
 * Return an index to device context.
 */
static uint8_t endpoint_dci(xhci_endpoint_t *ep)
{
	return (2 * ep->base.endpoint) +
		(ep->base.transfer_type == USB_TRANSFER_CONTROL
		 || ep->base.direction == USB_DIRECTION_IN);
}

void hc_ring_ep_doorbell(xhci_endpoint_t *ep, uint32_t stream_id)
{
	xhci_device_t * const dev = xhci_ep_to_dev(ep);
	xhci_hc_t * const hc = bus_to_hc(dev->base.bus);
	const uint8_t dci = endpoint_dci(ep);
	const uint32_t target = (stream_id << 16) | (dci & 0x1ff);
	hc_ring_doorbell(hc, dev->slot_id, target);
}

/**
 * Issue an Enable Slot command. Allocate memory for the slot and fill the
 * DCBAA with the newly created slot.
 */
errno_t hc_enable_slot(xhci_device_t *dev)
{
	errno_t err;
	xhci_hc_t * const hc = bus_to_hc(dev->base.bus);

	/* Prepare memory for the context */
	if ((err = dma_buffer_alloc(&dev->dev_ctx, XHCI_DEVICE_CTX_SIZE(hc))))
		return err;
	memset(dev->dev_ctx.virt, 0, XHCI_DEVICE_CTX_SIZE(hc));

	/* Get the slot number */
	xhci_cmd_t cmd;
	xhci_cmd_init(&cmd, XHCI_CMD_ENABLE_SLOT);

	err = xhci_cmd_sync(hc, &cmd);

	/* Link them together */
	if (err == EOK) {
		dev->slot_id = cmd.slot_id;
		hc->dcbaa[dev->slot_id] =
		    host2xhci(64, dma_buffer_phys_base(&dev->dev_ctx));
	}

	xhci_cmd_fini(&cmd);

	if (err)
		dma_buffer_free(&dev->dev_ctx);

	return err;
}

/**
 * Issue a Disable Slot command for a slot occupied by device.
 * Frees the device context.
 */
errno_t hc_disable_slot(xhci_device_t *dev)
{
	errno_t err;
	xhci_hc_t * const hc = bus_to_hc(dev->base.bus);

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

/**
 * Prepare an empty Endpoint Input Context inside a dma buffer.
 */
static errno_t create_configure_ep_input_ctx(xhci_device_t *dev, dma_buffer_t *dma_buf)
{
	const xhci_hc_t * hc = bus_to_hc(dev->base.bus);
	const errno_t err = dma_buffer_alloc(dma_buf, XHCI_INPUT_CTX_SIZE(hc));
	if (err)
		return err;

	xhci_input_ctx_t *ictx = dma_buf->virt;
	memset(ictx, 0, XHCI_INPUT_CTX_SIZE(hc));

	// Quoting sec. 4.6.5 and 4.6.6: A1, D0, D1 are down (already zeroed), A0 is up.
	XHCI_INPUT_CTRL_CTX_ADD_SET(*XHCI_GET_CTRL_CTX(ictx, hc), 0);
	xhci_slot_ctx_t *slot_ctx = XHCI_GET_SLOT_CTX(XHCI_GET_DEVICE_CTX(ictx, hc), hc);
	xhci_setup_slot_context(dev, slot_ctx);

	return EOK;
}

/**
 * Initialize a device, assigning it an address. Implements section 4.3.4.
 *
 * @param dev Device to assing an address (unconfigured yet)
 */
errno_t hc_address_device(xhci_device_t *dev)
{
	errno_t err = ENOMEM;
	xhci_hc_t * const hc = bus_to_hc(dev->base.bus);
	xhci_endpoint_t *ep0 = xhci_endpoint_get(dev->base.endpoints[0]);

	/* Although we have the precise PSIV value on devices of tier 1,
	 * we have to rely on reverse mapping on others. */
	if (!usb_speed_to_psiv[dev->base.speed]) {
		usb_log_error("Device reported an USB speed (%s) that cannot be mapped "
		    "to HC port speed.", usb_str_speed(dev->base.speed));
		return EINVAL;
	}

	/* Issue configure endpoint command (sec 4.3.5). */
	dma_buffer_t ictx_dma_buf;
	if ((err = create_configure_ep_input_ctx(dev, &ictx_dma_buf)))
		return err;
	xhci_input_ctx_t *ictx = ictx_dma_buf.virt;

	/* Copy endpoint 0 context and set A1 flag. */
	XHCI_INPUT_CTRL_CTX_ADD_SET(*XHCI_GET_CTRL_CTX(ictx, hc), 1);
	xhci_ep_ctx_t *ep_ctx = XHCI_GET_EP_CTX(XHCI_GET_DEVICE_CTX(ictx, hc), hc, 1);
	xhci_setup_endpoint_context(ep0, ep_ctx);

	/* Address device needs Ctx entries set to 1 only */
	xhci_slot_ctx_t *slot_ctx = XHCI_GET_SLOT_CTX(XHCI_GET_DEVICE_CTX(ictx, hc), hc);
	XHCI_SLOT_CTX_ENTRIES_SET(*slot_ctx, 1);

	/* Issue Address Device command. */
	if ((err = xhci_cmd_sync_inline(hc, ADDRESS_DEVICE,
		.slot_id = dev->slot_id,
		.input_ctx = ictx_dma_buf
	    )))
		return err;

	xhci_device_ctx_t *device_ctx = dev->dev_ctx.virt;
	dev->base.address = XHCI_SLOT_DEVICE_ADDRESS(*XHCI_GET_SLOT_CTX(device_ctx, hc));
	usb_log_debug("Obtained USB address: %d.", dev->base.address);

	return EOK;
}

/**
 * Issue a Configure Device command for a device in slot.
 *
 * @param slot_id Slot ID assigned to the device.
 */
errno_t hc_configure_device(xhci_device_t *dev)
{
	xhci_hc_t * const hc = bus_to_hc(dev->base.bus);

	/* Issue configure endpoint command (sec 4.3.5). */
	dma_buffer_t ictx_dma_buf;
	const errno_t err = create_configure_ep_input_ctx(dev, &ictx_dma_buf);
	if (err)
		return err;

	return xhci_cmd_sync_inline(hc, CONFIGURE_ENDPOINT,
		.slot_id = dev->slot_id,
		.input_ctx = ictx_dma_buf
	);
}

/**
 * Issue a Deconfigure Device command for a device in slot.
 *
 * @param dev The owner of the device
 */
errno_t hc_deconfigure_device(xhci_device_t *dev)
{
	xhci_hc_t * const hc = bus_to_hc(dev->base.bus);

	if (hc_is_broken(hc))
		return EOK;

	/* Issue configure endpoint command (sec 4.3.5) with the DC flag. */
	return xhci_cmd_sync_inline(hc, CONFIGURE_ENDPOINT,
		.slot_id = dev->slot_id,
		.deconfigure = true
	);
}

/**
 * Instruct xHC to add an endpoint with supplied endpoint context.
 *
 * @param dev The owner of the device
 * @param ep_idx Endpoint DCI in question
 * @param ep_ctx Endpoint context of the endpoint
 */
errno_t hc_add_endpoint(xhci_endpoint_t *ep)
{
	xhci_device_t * const dev = xhci_ep_to_dev(ep);
	const unsigned dci = endpoint_dci(ep);

	/* Issue configure endpoint command (sec 4.3.5). */
	dma_buffer_t ictx_dma_buf;
	const errno_t err = create_configure_ep_input_ctx(dev, &ictx_dma_buf);
	if (err)
		return err;

	xhci_input_ctx_t *ictx = ictx_dma_buf.virt;

	xhci_hc_t * const hc = bus_to_hc(dev->base.bus);
	XHCI_INPUT_CTRL_CTX_ADD_SET(*XHCI_GET_CTRL_CTX(ictx, hc), dci);

	xhci_ep_ctx_t *ep_ctx = XHCI_GET_EP_CTX(XHCI_GET_DEVICE_CTX(ictx, hc), hc, dci);
	xhci_setup_endpoint_context(ep, ep_ctx);

	return xhci_cmd_sync_inline(hc, CONFIGURE_ENDPOINT,
		.slot_id = dev->slot_id,
		.input_ctx = ictx_dma_buf
	);
}

/**
 * Instruct xHC to drop an endpoint.
 *
 * @param dev The owner of the endpoint
 * @param ep_idx Endpoint DCI in question
 */
errno_t hc_drop_endpoint(xhci_endpoint_t *ep)
{
	xhci_device_t * const dev = xhci_ep_to_dev(ep);
	xhci_hc_t * const hc = bus_to_hc(dev->base.bus);
	const unsigned dci = endpoint_dci(ep);

	if (hc_is_broken(hc))
		return EOK;

	/* Issue configure endpoint command (sec 4.3.5). */
	dma_buffer_t ictx_dma_buf;
	const errno_t err = create_configure_ep_input_ctx(dev, &ictx_dma_buf);
	if (err)
		return err;

	xhci_input_ctx_t *ictx = ictx_dma_buf.virt;
	XHCI_INPUT_CTRL_CTX_DROP_SET(*XHCI_GET_CTRL_CTX(ictx, hc), dci);

	return xhci_cmd_sync_inline(hc, CONFIGURE_ENDPOINT,
		.slot_id = dev->slot_id,
		.input_ctx = ictx_dma_buf
	);
}

/**
 * Instruct xHC to update information about an endpoint, using supplied
 * endpoint context.
 *
 * @param dev The owner of the endpoint
 * @param ep_idx Endpoint DCI in question
 * @param ep_ctx Endpoint context of the endpoint
 */
errno_t hc_update_endpoint(xhci_endpoint_t *ep)
{
	xhci_device_t * const dev = xhci_ep_to_dev(ep);
	const unsigned dci = endpoint_dci(ep);

	dma_buffer_t ictx_dma_buf;
	xhci_hc_t * const hc = bus_to_hc(dev->base.bus);

	const errno_t err = dma_buffer_alloc(&ictx_dma_buf, XHCI_INPUT_CTX_SIZE(hc));
	if (err)
		return err;

	xhci_input_ctx_t *ictx = ictx_dma_buf.virt;
	memset(ictx, 0, XHCI_INPUT_CTX_SIZE(hc));

	XHCI_INPUT_CTRL_CTX_ADD_SET(*XHCI_GET_CTRL_CTX(ictx, hc), dci);
	xhci_ep_ctx_t *ep_ctx = XHCI_GET_EP_CTX(XHCI_GET_DEVICE_CTX(ictx, hc), hc, dci);
	xhci_setup_endpoint_context(ep, ep_ctx);

	return xhci_cmd_sync_inline(hc, EVALUATE_CONTEXT,
		.slot_id = dev->slot_id,
		.input_ctx = ictx_dma_buf
	);
}

/**
 * Instruct xHC to stop running a transfer ring on an endpoint.
 *
 * @param dev The owner of the endpoint
 * @param ep_idx Endpoint DCI in question
 */
errno_t hc_stop_endpoint(xhci_endpoint_t *ep)
{
	xhci_device_t * const dev = xhci_ep_to_dev(ep);
	const unsigned dci = endpoint_dci(ep);
	xhci_hc_t * const hc = bus_to_hc(dev->base.bus);

	if (hc_is_broken(hc))
		return EOK;

	return xhci_cmd_sync_inline(hc, STOP_ENDPOINT,
		.slot_id = dev->slot_id,
		.endpoint_id = dci
	);
}

/**
 * Instruct xHC to reset halted endpoint.
 *
 * @param dev The owner of the endpoint
 * @param ep_idx Endpoint DCI in question
 */
errno_t hc_reset_endpoint(xhci_endpoint_t *ep)
{
	xhci_device_t * const dev = xhci_ep_to_dev(ep);
	const unsigned dci = endpoint_dci(ep);
	xhci_hc_t * const hc = bus_to_hc(dev->base.bus);
	return xhci_cmd_sync_inline(hc, RESET_ENDPOINT,
		.slot_id = dev->slot_id,
		.endpoint_id = dci
	);
}

/**
 * Reset a ring position in both software and hardware.
 *
 * @param dev The owner of the endpoint
 */
errno_t hc_reset_ring(xhci_endpoint_t *ep, uint32_t stream_id)
{
	xhci_device_t * const dev = xhci_ep_to_dev(ep);
	const unsigned dci = endpoint_dci(ep);
	uintptr_t addr;

	xhci_trb_ring_t *ring = xhci_endpoint_get_ring(ep, stream_id);
	xhci_trb_ring_reset_dequeue_state(ring, &addr);

	xhci_hc_t * const hc = bus_to_hc(endpoint_get_bus(&ep->base));
	return xhci_cmd_sync_inline(hc, SET_TR_DEQUEUE_POINTER,
		.slot_id = dev->slot_id,
		.endpoint_id = dci,
		.stream_id = stream_id,
		.dequeue_ptr = addr,
	);
}

/**
 * @}
 */
