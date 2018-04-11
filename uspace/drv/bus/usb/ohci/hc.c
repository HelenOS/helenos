/*
 * Copyright (c) 2011 Jan Vesely
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

/** @addtogroup drvusbohcihc
 * @{
 */
/** @file
 * @brief OHCI Host controller driver routines
 */

#include <assert.h>
#include <async.h>
#include <errno.h>
#include <macros.h>
#include <mem.h>
#include <stdlib.h>
#include <str_error.h>
#include <stddef.h>
#include <stdint.h>

#include <usb/debug.h>
#include <usb/host/utility.h>
#include <usb/usb.h>

#include "ohci_bus.h"
#include "ohci_batch.h"

#include "hc.h"

#define OHCI_USED_INTERRUPTS \
    (I_SO | I_WDH | I_UE | I_RHSC)

static const irq_pio_range_t ohci_pio_ranges[] = {
	{
		.base = 0,
		.size = sizeof(ohci_regs_t)
	}
};

static const irq_cmd_t ohci_irq_commands[] = {
	{
		.cmd = CMD_PIO_READ_32,
		.dstarg = 1,
		.addr = NULL
	},
	{
		.cmd = CMD_AND,
		.srcarg = 1,
		.dstarg = 2,
		.value = 0
	},
	{
		.cmd = CMD_PREDICATE,
		.srcarg = 2,
		.value = 2
	},
	{
		.cmd = CMD_PIO_WRITE_A_32,
		.srcarg = 1,
		.addr = NULL
	},
	{
		.cmd = CMD_ACCEPT
	}
};

static errno_t hc_init_transfer_lists(hc_t *instance);
static errno_t hc_init_memory(hc_t *instance);

/** Generate IRQ code.
 * @param[out] ranges PIO ranges buffer.
 * @param[in] ranges_size Size of the ranges buffer (bytes).
 * @param[out] cmds Commands buffer.
 * @param[in] cmds_size Size of the commands buffer (bytes).
 * @param[in] hw_res Device's resources.
 *
 * @return Error code.
 */
errno_t hc_gen_irq_code(irq_code_t *code, hc_device_t *hcd, const hw_res_list_parsed_t *hw_res, int *irq)
{
	assert(code);
	assert(hw_res);

	if (hw_res->irqs.count != 1 || hw_res->mem_ranges.count != 1)
		return EINVAL;

	const addr_range_t regs = hw_res->mem_ranges.ranges[0];

	if (RNGSZ(regs) < sizeof(ohci_regs_t))
		return EOVERFLOW;

	code->ranges = malloc(sizeof(ohci_pio_ranges));
	if (code->ranges == NULL)
		return ENOMEM;

	code->cmds = malloc(sizeof(ohci_irq_commands));
	if (code->cmds == NULL) {
		free(code->ranges);
		return ENOMEM;
	}

	code->rangecount = ARRAY_SIZE(ohci_pio_ranges);
	code->cmdcount = ARRAY_SIZE(ohci_irq_commands);

	memcpy(code->ranges, ohci_pio_ranges, sizeof(ohci_pio_ranges));
	code->ranges[0].base = RNGABS(regs);

	memcpy(code->cmds, ohci_irq_commands, sizeof(ohci_irq_commands));
	ohci_regs_t *registers = (ohci_regs_t *) RNGABSPTR(regs);
	code->cmds[0].addr = (void *) &registers->interrupt_status;
	code->cmds[3].addr = (void *) &registers->interrupt_status;
	OHCI_WR(code->cmds[1].value, OHCI_USED_INTERRUPTS);

	usb_log_debug("Memory mapped regs at %p (size %zu), IRQ %d.",
	    RNGABSPTR(regs), RNGSZ(regs), hw_res->irqs.irqs[0]);

	*irq = hw_res->irqs.irqs[0];
	return EOK;
}

/** Initialize OHCI hc driver structure
 *
 * @param[in] instance Memory place for the structure.
 * @param[in] regs Device's resources
 * @param[in] interrupts True if w interrupts should be used
 * @return Error code
 */
errno_t hc_add(hc_device_t *hcd, const hw_res_list_parsed_t *hw_res)
{
	hc_t *instance = hcd_to_hc(hcd);
	assert(hw_res);
	if (hw_res->mem_ranges.count != 1 ||
	    hw_res->mem_ranges.ranges[0].size < sizeof(ohci_regs_t))
		return EINVAL;

	errno_t ret = pio_enable_range(&hw_res->mem_ranges.ranges[0],
	    (void **) &instance->registers);
	if (ret != EOK) {
		usb_log_error("Failed to gain access to registers: %s.",
		    str_error(ret));
		return ret;
	}
	usb_log_debug("Device registers at %" PRIx64 " (%zuB) accessible.",
	    hw_res->mem_ranges.ranges[0].address.absolute,
	    hw_res->mem_ranges.ranges[0].size);

	list_initialize(&instance->pending_endpoints);
	fibril_mutex_initialize(&instance->guard);

	ret = hc_init_memory(instance);
	if (ret != EOK) {
		usb_log_error("Failed to create OHCI memory structures: %s.",
		    str_error(ret));
		// TODO: We should disable pio access here
		return ret;
	}

	return EOK;
}

/** Safely dispose host controller internal structures
 *
 * @param[in] instance Host controller structure to use.
 */
int hc_gone(hc_device_t *instance)
{
	assert(instance);
	/* TODO: implement*/
	return ENOTSUP;
}

void hc_enqueue_endpoint(hc_t *instance, const endpoint_t *ep)
{
	assert(instance);
	assert(ep);

	endpoint_list_t *list = &instance->lists[ep->transfer_type];
	ohci_endpoint_t *ohci_ep = ohci_endpoint_get(ep);
	assert(list);
	assert(ohci_ep);

	/* Enqueue ep */
	switch (ep->transfer_type) {
	case USB_TRANSFER_CONTROL:
		OHCI_CLR(instance->registers->control, C_CLE);
		endpoint_list_add_ep(list, ohci_ep);
		OHCI_WR(instance->registers->control_current, 0);
		OHCI_SET(instance->registers->control, C_CLE);
		break;
	case USB_TRANSFER_BULK:
		OHCI_CLR(instance->registers->control, C_BLE);
		endpoint_list_add_ep(list, ohci_ep);
		OHCI_WR(instance->registers->bulk_current, 0);
		OHCI_SET(instance->registers->control, C_BLE);
		break;
	case USB_TRANSFER_ISOCHRONOUS:
	case USB_TRANSFER_INTERRUPT:
		OHCI_CLR(instance->registers->control, C_PLE | C_IE);
		endpoint_list_add_ep(list, ohci_ep);
		OHCI_SET(instance->registers->control, C_PLE | C_IE);
		break;
	}
}

void hc_dequeue_endpoint(hc_t *instance, const endpoint_t *ep)
{
	assert(instance);
	assert(ep);

	/* Dequeue ep */
	endpoint_list_t *list = &instance->lists[ep->transfer_type];
	ohci_endpoint_t *ohci_ep = ohci_endpoint_get(ep);

	assert(list);
	assert(ohci_ep);
	switch (ep->transfer_type) {
	case USB_TRANSFER_CONTROL:
		OHCI_CLR(instance->registers->control, C_CLE);
		endpoint_list_remove_ep(list, ohci_ep);
		OHCI_WR(instance->registers->control_current, 0);
		OHCI_SET(instance->registers->control, C_CLE);
		break;
	case USB_TRANSFER_BULK:
		OHCI_CLR(instance->registers->control, C_BLE);
		endpoint_list_remove_ep(list, ohci_ep);
		OHCI_WR(instance->registers->bulk_current, 0);
		OHCI_SET(instance->registers->control, C_BLE);
		break;
	case USB_TRANSFER_ISOCHRONOUS:
	case USB_TRANSFER_INTERRUPT:
		OHCI_CLR(instance->registers->control, C_PLE | C_IE);
		endpoint_list_remove_ep(list, ohci_ep);
		OHCI_SET(instance->registers->control, C_PLE | C_IE);
		break;
	default:
		break;
	}
}

errno_t ohci_hc_status(bus_t *bus_base, uint32_t *status)
{
	assert(bus_base);
	assert(status);

	ohci_bus_t *bus = (ohci_bus_t *) bus_base;
	hc_t *hc = bus->hc;
	assert(hc);

	if (hc->registers) {
		*status = OHCI_RD(hc->registers->interrupt_status);
		OHCI_WR(hc->registers->interrupt_status, *status);
	}
	return EOK;
}

/** Add USB transfer to the schedule.
 *
 * @param[in] hcd HCD driver structure.
 * @param[in] batch Batch representing the transfer.
 * @return Error code.
 */
errno_t ohci_hc_schedule(usb_transfer_batch_t *batch)
{
	assert(batch);

	ohci_bus_t *bus = (ohci_bus_t *) endpoint_get_bus(batch->ep);
	hc_t *hc = bus->hc;
	assert(hc);

	/* Check for root hub communication */
	if (batch->target.address == ohci_rh_get_address(&hc->rh)) {
		usb_log_debug("OHCI root hub request.");
		return ohci_rh_schedule(&hc->rh, batch);
	}

	endpoint_t *ep = batch->ep;
	ohci_endpoint_t *const ohci_ep = ohci_endpoint_get(ep);
	ohci_transfer_batch_t *ohci_batch = ohci_transfer_batch_get(batch);
	int err;

	if ((err = ohci_transfer_batch_prepare(ohci_batch)))
		return err;

	fibril_mutex_lock(&hc->guard);
	if ((err = endpoint_activate_locked(ep, batch))) {
		fibril_mutex_unlock(&hc->guard);
		return err;
	}

	ohci_transfer_batch_commit(ohci_batch);
	list_append(&ohci_ep->pending_link, &hc->pending_endpoints);
	fibril_mutex_unlock(&hc->guard);

	/* Control and bulk schedules need a kick to start working */
	switch (batch->ep->transfer_type) {
	case USB_TRANSFER_CONTROL:
		OHCI_SET(hc->registers->command_status, CS_CLF);
		break;
	case USB_TRANSFER_BULK:
		OHCI_SET(hc->registers->command_status, CS_BLF);
		break;
	default:
		break;
	}

	return EOK;
}

/** Interrupt handling routine
 *
 * @param[in] hcd HCD driver structure.
 * @param[in] status Value of the status register at the time of interrupt.
 */
void ohci_hc_interrupt(bus_t *bus_base, uint32_t status)
{
	assert(bus_base);

	ohci_bus_t *bus = (ohci_bus_t *) bus_base;
	hc_t *hc = bus->hc;
	assert(hc);

	status = OHCI_RD(status);
	assert(hc);
	if ((status & ~I_SF) == 0) /* ignore sof status */
		return;
	usb_log_debug2("OHCI(%p) interrupt: %x.", hc, status);
	if (status & I_RHSC)
		ohci_rh_interrupt(&hc->rh);

	if (status & I_WDH) {
		fibril_mutex_lock(&hc->guard);
		usb_log_debug2("HCCA: %p-%#" PRIx32 " (%p).", hc->hcca,
		    OHCI_RD(hc->registers->hcca),
		    (void *) addr_to_phys(hc->hcca));
		usb_log_debug2("Periodic current: %#" PRIx32 ".",
		    OHCI_RD(hc->registers->periodic_current));

		list_foreach_safe(hc->pending_endpoints, current, next) {
			ohci_endpoint_t *ep =
			    list_get_instance(current, ohci_endpoint_t, pending_link);

			ohci_transfer_batch_t *batch =
			    ohci_transfer_batch_get(ep->base.active_batch);
			assert(batch);

			if (ohci_transfer_batch_check_completed(batch)) {
				endpoint_deactivate_locked(&ep->base);
				list_remove(current);
				hc_reset_toggles(&batch->base, &ohci_ep_toggle_reset);
				usb_transfer_batch_finish(&batch->base);
			}
		}
		fibril_mutex_unlock(&hc->guard);
	}

	if (status & I_UE) {
		usb_log_fatal("Error like no other!");
		hc_start(&hc->base);
	}

}

/** Turn off any (BIOS)driver that might be in control of the device.
 *
 * This function implements routines described in chapter 5.1.1.3 of the OHCI
 * specification (page 40, pdf page 54).
 *
 * @param[in] instance OHCI hc driver structure.
 */
int hc_gain_control(hc_device_t *hcd)
{
	hc_t *instance = hcd_to_hc(hcd);

	usb_log_debug("Requesting OHCI control.");
	if (OHCI_RD(instance->registers->revision) & R_LEGACY_FLAG) {
		/* Turn off legacy emulation, it should be enough to zero
		 * the lowest bit, but it caused problems. Thus clear all
		 * except GateA20 (causes restart on some hw).
		 * See page 145 of the specs for details.
		 */
		volatile uint32_t *ohci_emulation_reg =
		    (uint32_t *)((char *)instance->registers + LEGACY_REGS_OFFSET);
		usb_log_debug("OHCI legacy register %p: %x.",
		    ohci_emulation_reg, OHCI_RD(*ohci_emulation_reg));
		/* Zero everything but A20State */
		// TODO: should we ack interrupts before doing this?
		OHCI_CLR(*ohci_emulation_reg, ~0x100);
		usb_log_debug(
		    "OHCI legacy register (should be 0 or 0x100) %p: %x.",
		    ohci_emulation_reg, OHCI_RD(*ohci_emulation_reg));
	}

	/* Interrupt routing enabled => smm driver is active */
	if (OHCI_RD(instance->registers->control) & C_IR) {
		usb_log_debug("SMM driver: request ownership change.");
		// TODO: should we ack interrupts before doing this?
		OHCI_SET(instance->registers->command_status, CS_OCR);
		/* Hope that SMM actually knows its stuff or we can hang here */
		while (OHCI_RD(instance->registers->control) & C_IR) {
			async_usleep(1000);
		}
		usb_log_info("SMM driver: Ownership taken.");
		C_HCFS_SET(instance->registers->control, C_HCFS_RESET);
		async_usleep(50000);
		return EOK;
	}

	const unsigned hc_status = C_HCFS_GET(instance->registers->control);
	/* Interrupt routing disabled && status != USB_RESET => BIOS active */
	if (hc_status != C_HCFS_RESET) {
		usb_log_debug("BIOS driver found.");
		if (hc_status == C_HCFS_OPERATIONAL) {
			usb_log_info("BIOS driver: HC operational.");
			return EOK;
		}
		/* HC is suspended assert resume for 20ms */
		C_HCFS_SET(instance->registers->control, C_HCFS_RESUME);
		async_usleep(20000);
		usb_log_info("BIOS driver: HC resumed.");
		return EOK;
	}

	/* HC is in reset (hw startup) => no other driver
	 * maintain reset for at least the time specified in USB spec (50 ms)*/
	usb_log_debug("Host controller found in reset state.");
	async_usleep(50000);
	return EOK;
}

/** OHCI hw initialization routine.
 *
 * @param[in] instance OHCI hc driver structure.
 */
int hc_start(hc_device_t *hcd)
{
	hc_t *instance = hcd_to_hc(hcd);
	ohci_rh_init(&instance->rh, instance->registers, &instance->guard, "ohci rh");

	/* OHCI guide page 42 */
	assert(instance);
	usb_log_debug2("Started hc initialization routine.");

	/* Save contents of fm_interval register */
	const uint32_t fm_interval = OHCI_RD(instance->registers->fm_interval);
	usb_log_debug2("Old value of HcFmInterval: %x.", fm_interval);

	/* Reset hc */
	usb_log_debug2("HC reset.");
	size_t time = 0;
	OHCI_WR(instance->registers->command_status, CS_HCR);
	while (OHCI_RD(instance->registers->command_status) & CS_HCR) {
		async_usleep(10);
		time += 10;
	}
	usb_log_debug2("HC reset complete in %zu us.", time);

	/* Restore fm_interval */
	OHCI_WR(instance->registers->fm_interval, fm_interval);
	assert((OHCI_RD(instance->registers->command_status) & CS_HCR) == 0);

	/* hc is now in suspend state */
	usb_log_debug2("HC should be in suspend state(%x).",
	    OHCI_RD(instance->registers->control));

	/* Use HCCA */
	OHCI_WR(instance->registers->hcca, addr_to_phys(instance->hcca));

	/* Use queues */
	OHCI_WR(instance->registers->bulk_head,
	    instance->lists[USB_TRANSFER_BULK].list_head_pa);
	usb_log_debug2("Bulk HEAD set to: %p (%#" PRIx32 ").",
	    instance->lists[USB_TRANSFER_BULK].list_head,
	    instance->lists[USB_TRANSFER_BULK].list_head_pa);

	OHCI_WR(instance->registers->control_head,
	    instance->lists[USB_TRANSFER_CONTROL].list_head_pa);
	usb_log_debug2("Control HEAD set to: %p (%#" PRIx32 ").",
	    instance->lists[USB_TRANSFER_CONTROL].list_head,
	    instance->lists[USB_TRANSFER_CONTROL].list_head_pa);

	/* Enable queues */
	OHCI_SET(instance->registers->control, (C_PLE | C_IE | C_CLE | C_BLE));
	usb_log_debug("Queues enabled(%x).",
	    OHCI_RD(instance->registers->control));

	/* Enable interrupts */
	if (CAP_HANDLE_VALID(instance->base.irq_handle)) {
		OHCI_WR(instance->registers->interrupt_enable,
		    OHCI_USED_INTERRUPTS);
		usb_log_debug("Enabled interrupts: %x.",
		    OHCI_RD(instance->registers->interrupt_enable));
		OHCI_WR(instance->registers->interrupt_enable, I_MI);
	}

	/* Set periodic start to 90% */
	const uint32_t frame_length =
	    (fm_interval >> FMI_FI_SHIFT) & FMI_FI_MASK;
	OHCI_WR(instance->registers->periodic_start,
	    ((frame_length / 10) * 9) & PS_MASK << PS_SHIFT);
	usb_log_debug2("All periodic start set to: %x(%u - 90%% of %d).",
	    OHCI_RD(instance->registers->periodic_start),
	    OHCI_RD(instance->registers->periodic_start), frame_length);
	C_HCFS_SET(instance->registers->control, C_HCFS_OPERATIONAL);
	usb_log_debug("OHCI HC up and running (ctl_reg=0x%x).",
	    OHCI_RD(instance->registers->control));

	return EOK;
}

/**
 * Setup roothub as a virtual hub.
 */
int hc_setup_roothub(hc_device_t *hcd)
{
	return hc_setup_virtual_root_hub(hcd, USB_SPEED_FULL);
}

/** Initialize schedule queues
 *
 * @param[in] instance OHCI hc driver structure
 * @return Error code
 */
errno_t hc_init_transfer_lists(hc_t *instance)
{
	assert(instance);
#define SETUP_ENDPOINT_LIST(type) \
do { \
	const char *name = usb_str_transfer_type(type); \
	const errno_t ret = endpoint_list_init(&instance->lists[type], name); \
	if (ret != EOK) { \
		usb_log_error("Failed to setup %s endpoint list: %s.", \
		    name, str_error(ret)); \
		endpoint_list_fini(&instance->lists[USB_TRANSFER_ISOCHRONOUS]);\
		endpoint_list_fini(&instance->lists[USB_TRANSFER_INTERRUPT]); \
		endpoint_list_fini(&instance->lists[USB_TRANSFER_CONTROL]); \
		endpoint_list_fini(&instance->lists[USB_TRANSFER_BULK]); \
		return ret; \
	} \
} while (0)

	SETUP_ENDPOINT_LIST(USB_TRANSFER_ISOCHRONOUS);
	SETUP_ENDPOINT_LIST(USB_TRANSFER_INTERRUPT);
	SETUP_ENDPOINT_LIST(USB_TRANSFER_CONTROL);
	SETUP_ENDPOINT_LIST(USB_TRANSFER_BULK);
#undef SETUP_ENDPOINT_LIST
	endpoint_list_set_next(&instance->lists[USB_TRANSFER_INTERRUPT],
	    &instance->lists[USB_TRANSFER_ISOCHRONOUS]);

	return EOK;
}

/** Initialize memory structures used by the OHCI hcd.
 *
 * @param[in] instance OHCI hc driver structure.
 * @return Error code.
 */
errno_t hc_init_memory(hc_t *instance)
{
	assert(instance);

	memset(&instance->rh, 0, sizeof(instance->rh));
	/* Init queues */
	errno_t ret = hc_init_transfer_lists(instance);
	if (ret != EOK) {
		return ret;
	}

	/*Init HCCA */
	instance->hcca = hcca_get();
	if (instance->hcca == NULL)
		return ENOMEM;
	usb_log_debug2("OHCI HCCA initialized at %p.", instance->hcca);

	for (unsigned i = 0; i < HCCA_INT_EP_COUNT; ++i) {
		hcca_set_int_ep(instance->hcca, i,
		    instance->lists[USB_TRANSFER_INTERRUPT].list_head_pa);
	}
	usb_log_debug2("Interrupt HEADs set to: %p (%#" PRIx32 ").",
	    instance->lists[USB_TRANSFER_INTERRUPT].list_head,
	    instance->lists[USB_TRANSFER_INTERRUPT].list_head_pa);

	if ((ret = ohci_bus_init(&instance->bus, instance))) {
		usb_log_error("HC(%p): Failed to setup bus : %s",
		    instance, str_error(ret));
		return ret;
	}

	hc_device_setup(&instance->base, (bus_t *) &instance->bus);

	return EOK;
}

/**
 * @}
 */
