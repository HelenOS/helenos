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

/** @addtogroup drvusbehcihc
 * @{
 */
/** @file
 * @brief EHCI Host controller driver routines
 */

#include <assert.h>
#include <async.h>
#include <errno.h>
#include <macros.h>
#include <mem.h>
#include <stdlib.h>
#include <stdint.h>
#include <str_error.h>

#include <usb/debug.h>
#include <usb/usb.h>
#include <usb/host/utility.h>

#include "ehci_batch.h"

#include "hc.h"

#define EHCI_USED_INTERRUPTS \
    (USB_INTR_IRQ_FLAG | USB_INTR_ERR_IRQ_FLAG | USB_INTR_PORT_CHANGE_FLAG | \
    USB_INTR_ASYNC_ADVANCE_FLAG | USB_INTR_HOST_ERR_FLAG)

static const irq_pio_range_t ehci_pio_ranges[] = {
	{
		.base = 0,
		.size = sizeof(ehci_regs_t)
	}
};

static const irq_cmd_t ehci_irq_commands[] = {
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

static errno_t hc_init_memory(hc_t *instance);

/** Generate IRQ code.
 * @param[out] ranges PIO ranges buffer.
 * @param[in] hw_res Device's resources.
 *
 * @param[out] irq
 *
 * @return Error code.
 */
errno_t hc_gen_irq_code(irq_code_t *code, hc_device_t *hcd, const hw_res_list_parsed_t *hw_res, int *irq)
{
	assert(code);
	assert(hw_res);
	hc_t *instance = hcd_to_hc(hcd);

	if (hw_res->irqs.count != 1 || hw_res->mem_ranges.count != 1)
		return EINVAL;

	addr_range_t regs = hw_res->mem_ranges.ranges[0];

	if (RNGSZ(regs) < sizeof(ehci_regs_t))
		return EOVERFLOW;

	code->ranges = malloc(sizeof(ehci_pio_ranges));
	if (code->ranges == NULL)
		return ENOMEM;

	code->cmds = malloc(sizeof(ehci_irq_commands));
	if (code->cmds == NULL) {
		free(code->ranges);
		return ENOMEM;
	}

	code->rangecount = ARRAY_SIZE(ehci_pio_ranges);
	code->cmdcount = ARRAY_SIZE(ehci_irq_commands);

	memcpy(code->ranges, ehci_pio_ranges, sizeof(ehci_pio_ranges));
	code->ranges[0].base = RNGABS(regs);

	memcpy(code->cmds, ehci_irq_commands, sizeof(ehci_irq_commands));

	ehci_regs_t *registers =
	    (ehci_regs_t *)(RNGABSPTR(regs) + EHCI_RD8(instance->caps->caplength));
	code->cmds[0].addr = (void *) &registers->usbsts;
	code->cmds[3].addr = (void *) &registers->usbsts;
	EHCI_WR(code->cmds[1].value, EHCI_USED_INTERRUPTS);

	usb_log_debug("Memory mapped regs at %p (size %zu), IRQ %d.",
	    RNGABSPTR(regs), RNGSZ(regs), hw_res->irqs.irqs[0]);

	*irq = hw_res->irqs.irqs[0];
	return EOK;
}

/** Initialize EHCI hc driver structure
 *
 * @param[in] instance Memory place for the structure.
 * @param[in] regs Device's I/O registers range.
 * @param[in] interrupts True if w interrupts should be used
 * @return Error code
 */
errno_t hc_add(hc_device_t *hcd, const hw_res_list_parsed_t *hw_res)
{
	hc_t *instance = hcd_to_hc(hcd);
	assert(hw_res);
	if (hw_res->mem_ranges.count != 1 ||
	    hw_res->mem_ranges.ranges[0].size <
	    (sizeof(ehci_caps_regs_t) + sizeof(ehci_regs_t)))
		return EINVAL;

	errno_t ret = pio_enable_range(&hw_res->mem_ranges.ranges[0],
	    (void **)&instance->caps);
	if (ret != EOK) {
		usb_log_error("HC(%p): Failed to gain access to device "
		    "registers: %s.", instance, str_error(ret));
		return ret;
	}

	usb_log_info("HC(%p): Device registers at %" PRIx64 " (%zuB) accessible.",
	    instance, hw_res->mem_ranges.ranges[0].address.absolute,
	    hw_res->mem_ranges.ranges[0].size);
	instance->registers =
	    (void *)instance->caps + EHCI_RD8(instance->caps->caplength);
	usb_log_info("HC(%p): Device control registers at %" PRIx64, instance,
	    hw_res->mem_ranges.ranges[0].address.absolute +
	    EHCI_RD8(instance->caps->caplength));

	list_initialize(&instance->pending_endpoints);
	fibril_mutex_initialize(&instance->guard);
	fibril_condvar_initialize(&instance->async_doorbell);

	ret = hc_init_memory(instance);
	if (ret != EOK) {
		usb_log_error("HC(%p): Failed to create EHCI memory structures:"
		    " %s.", instance, str_error(ret));
		return ret;
	}

	usb_log_info("HC(%p): Initializing RH(%p).", instance, &instance->rh);
	ehci_rh_init(
	    &instance->rh, instance->caps, instance->registers, &instance->guard,
	    "ehci rh");

	ehci_bus_init(&instance->bus, instance);
	hc_device_setup(hcd, (bus_t *) &instance->bus);
	return EOK;
}

/** Safely dispose host controller internal structures
 *
 * @param[in] instance Host controller structure to use.
 */
int hc_gone(hc_device_t *hcd)
{
	hc_t *hc = hcd_to_hc(hcd);
	endpoint_list_fini(&hc->async_list);
	endpoint_list_fini(&hc->int_list);
	dma_buffer_free(&hc->dma_buffer);
	return EOK;
}

void hc_enqueue_endpoint(hc_t *instance, const endpoint_t *ep)
{
	assert(instance);
	assert(ep);
	ehci_endpoint_t *ehci_ep = ehci_endpoint_get(ep);
	usb_log_debug("HC(%p) enqueue EP(%d:%d:%s:%s)", instance,
	    ep->device->address, ep->endpoint,
	    usb_str_transfer_type_short(ep->transfer_type),
	    usb_str_direction(ep->direction));
	switch (ep->transfer_type) {
	case USB_TRANSFER_CONTROL:
	case USB_TRANSFER_BULK:
		endpoint_list_append_ep(&instance->async_list, ehci_ep);
		break;
	case USB_TRANSFER_INTERRUPT:
		endpoint_list_append_ep(&instance->int_list, ehci_ep);
		break;
	case USB_TRANSFER_ISOCHRONOUS:
		/* NOT SUPPORTED */
		break;
	}
}

void hc_dequeue_endpoint(hc_t *instance, const endpoint_t *ep)
{
	assert(instance);
	assert(ep);
	ehci_endpoint_t *ehci_ep = ehci_endpoint_get(ep);
	usb_log_debug("HC(%p) dequeue EP(%d:%d:%s:%s)", instance,
	    ep->device->address, ep->endpoint,
	    usb_str_transfer_type_short(ep->transfer_type),
	    usb_str_direction(ep->direction));
	switch (ep->transfer_type) {
	case USB_TRANSFER_INTERRUPT:
		endpoint_list_remove_ep(&instance->int_list, ehci_ep);
		/* Fall through */
	case USB_TRANSFER_ISOCHRONOUS:
		/* NOT SUPPORTED */
		return;
	case USB_TRANSFER_CONTROL:
	case USB_TRANSFER_BULK:
		endpoint_list_remove_ep(&instance->async_list, ehci_ep);
		break;
	}
	fibril_mutex_lock(&instance->guard);
	usb_log_debug("HC(%p): Waiting for doorbell", instance);
	EHCI_SET(instance->registers->usbcmd, USB_CMD_IRQ_ASYNC_DOORBELL);
	fibril_condvar_wait(&instance->async_doorbell, &instance->guard);
	usb_log_debug2("HC(%p): Got doorbell", instance);
	fibril_mutex_unlock(&instance->guard);
}

errno_t ehci_hc_status(bus_t *bus_base, uint32_t *status)
{
	assert(bus_base);
	assert(status);

	ehci_bus_t *bus = (ehci_bus_t *) bus_base;
	hc_t *hc = bus->hc;
	assert(hc);

	*status = 0;
	if (hc->registers) {
		*status = EHCI_RD(hc->registers->usbsts);
		EHCI_WR(hc->registers->usbsts, *status);
	}
	usb_log_debug2("HC(%p): Read status: %x", hc, *status);
	return EOK;
}

/** Add USB transfer to the schedule.
 *
 * @param[in] hcd HCD driver structure.
 * @param[in] batch Batch representing the transfer.
 * @return Error code.
 */
errno_t ehci_hc_schedule(usb_transfer_batch_t *batch)
{
	assert(batch);

	ehci_bus_t *bus = (ehci_bus_t *) endpoint_get_bus(batch->ep);
	hc_t *hc = bus->hc;
	assert(hc);

	/* Check for root hub communication */
	if (batch->target.address == ehci_rh_get_address(&hc->rh)) {
		usb_log_debug("HC(%p): Scheduling BATCH(%p) for RH(%p)",
		    hc, batch, &hc->rh);
		return ehci_rh_schedule(&hc->rh, batch);
	}

	endpoint_t *const ep = batch->ep;
	ehci_endpoint_t *const ehci_ep = ehci_endpoint_get(ep);
	ehci_transfer_batch_t *ehci_batch = ehci_transfer_batch_get(batch);

	int err;

	if ((err = ehci_transfer_batch_prepare(ehci_batch)))
		return err;

	fibril_mutex_lock(&hc->guard);

	if ((err = endpoint_activate_locked(ep, batch))) {
		fibril_mutex_unlock(&hc->guard);
		return err;
	}

	usb_log_debug("HC(%p): Committing BATCH(%p)", hc, batch);
	ehci_transfer_batch_commit(ehci_batch);

	/* Enqueue endpoint to the checked list */
	usb_log_debug2("HC(%p): Appending BATCH(%p)", hc, batch);
	list_append(&ehci_ep->pending_link, &hc->pending_endpoints);

	fibril_mutex_unlock(&hc->guard);
	return EOK;
}

/** Interrupt handling routine
 *
 * @param[in] hcd HCD driver structure.
 * @param[in] status Value of the status register at the time of interrupt.
 */
void ehci_hc_interrupt(bus_t *bus_base, uint32_t status)
{
	assert(bus_base);

	ehci_bus_t *bus = (ehci_bus_t *) bus_base;
	hc_t *hc = bus->hc;
	assert(hc);

	usb_log_debug2("HC(%p): Interrupt: %" PRIx32, hc, status);
	if (status & USB_STS_PORT_CHANGE_FLAG) {
		ehci_rh_interrupt(&hc->rh);
	}

	if (status & USB_STS_IRQ_ASYNC_ADVANCE_FLAG) {
		fibril_mutex_lock(&hc->guard);
		usb_log_debug2("HC(%p): Signaling doorbell", hc);
		fibril_condvar_broadcast(&hc->async_doorbell);
		fibril_mutex_unlock(&hc->guard);
	}

	if (status & (USB_STS_IRQ_FLAG | USB_STS_ERR_IRQ_FLAG)) {
		fibril_mutex_lock(&hc->guard);

		usb_log_debug2("HC(%p): Scanning %lu pending endpoints", hc,
		    list_count(&hc->pending_endpoints));
		list_foreach_safe(hc->pending_endpoints, current, next) {
			ehci_endpoint_t *ep =
			    list_get_instance(current, ehci_endpoint_t, pending_link);

			ehci_transfer_batch_t *batch =
			    ehci_transfer_batch_get(ep->base.active_batch);
			assert(batch);

			if (ehci_transfer_batch_check_completed(batch)) {
				endpoint_deactivate_locked(&ep->base);
				list_remove(current);
				hc_reset_toggles(&batch->base, &ehci_ep_toggle_reset);
				usb_transfer_batch_finish(&batch->base);
			}
		}
		fibril_mutex_unlock(&hc->guard);


	}

	if (status & USB_STS_HOST_ERROR_FLAG) {
		usb_log_fatal("HCD(%p): HOST SYSTEM ERROR!", hc);
		//TODO do something here
	}
}

/** EHCI hw initialization routine.
 *
 * @param[in] instance EHCI hc driver structure.
 */
int hc_start(hc_device_t *hcd)
{
	hc_t *instance = hcd_to_hc(hcd);
	usb_log_debug("HC(%p): Starting HW.", instance);

	/* Turn off the HC if it's running, Reseting a running device is
	 * undefined */
	if (!(EHCI_RD(instance->registers->usbsts) & USB_STS_HC_HALTED_FLAG)) {
		/* disable all interrupts */
		EHCI_WR(instance->registers->usbintr, 0);
		/* ack all interrupts */
		EHCI_WR(instance->registers->usbsts, 0x3f);
		/* Stop HC hw */
		EHCI_WR(instance->registers->usbcmd, 0);
		/* Wait until hc is halted */
		while ((EHCI_RD(instance->registers->usbsts) & USB_STS_HC_HALTED_FLAG) == 0) {
			async_usleep(1);
		}
		usb_log_info("HC(%p): EHCI turned off.", instance);
	} else {
		usb_log_info("HC(%p): EHCI was not running.", instance);
	}

	/* Hw initialization sequence, see page 53 (pdf 63) */
	EHCI_SET(instance->registers->usbcmd, USB_CMD_HC_RESET_FLAG);
	usb_log_info("HC(%p): Waiting for HW reset.", instance);
	while (EHCI_RD(instance->registers->usbcmd) & USB_CMD_HC_RESET_FLAG) {
		async_usleep(1);
	}
	usb_log_debug("HC(%p): HW reset OK.", instance);

	/* Use the lowest 4G segment */
	EHCI_WR(instance->registers->ctrldssegment, 0);

	/* Enable periodic list */
	assert(instance->periodic_list);
	uintptr_t phys_base =
	    addr_to_phys((void *)instance->periodic_list);
	assert((phys_base & USB_PERIODIC_LIST_BASE_MASK) == phys_base);
	EHCI_WR(instance->registers->periodiclistbase, phys_base);
	EHCI_SET(instance->registers->usbcmd, USB_CMD_PERIODIC_SCHEDULE_FLAG);
	usb_log_debug("HC(%p): Enabled periodic list.", instance);


	/* Enable Async schedule */
	phys_base = addr_to_phys((void *)instance->async_list.list_head);
	assert((phys_base & USB_ASYNCLIST_MASK) == phys_base);
	EHCI_WR(instance->registers->asynclistaddr, phys_base);
	EHCI_SET(instance->registers->usbcmd, USB_CMD_ASYNC_SCHEDULE_FLAG);
	usb_log_debug("HC(%p): Enabled async list.", instance);

	/* Start hc and get all ports */
	EHCI_SET(instance->registers->usbcmd, USB_CMD_RUN_FLAG);
	EHCI_SET(instance->registers->configflag, USB_CONFIG_FLAG_FLAG);
	usb_log_debug("HC(%p): HW started.", instance);

	usb_log_debug2("HC(%p): Registers: "
	    "\tUSBCMD(%p): %x(0x00080000 = at least 1ms between interrupts)"
	    "\tUSBSTS(%p): %x(0x00001000 = HC halted)"
	    "\tUSBINT(%p): %x(0x0 = no interrupts)."
	    "\tCONFIG(%p): %x(0x0 = ports controlled by companion hc).",
	    instance,
	    &instance->registers->usbcmd, EHCI_RD(instance->registers->usbcmd),
	    &instance->registers->usbsts, EHCI_RD(instance->registers->usbsts),
	    &instance->registers->usbintr, EHCI_RD(instance->registers->usbintr),
	    &instance->registers->configflag, EHCI_RD(instance->registers->configflag));
	/* Clear and Enable interrupts */
	EHCI_WR(instance->registers->usbsts, EHCI_RD(instance->registers->usbsts));
	EHCI_WR(instance->registers->usbintr, EHCI_USED_INTERRUPTS);

	return EOK;
}

/**
 * Setup roothub as a virtual hub.
 */
int hc_setup_roothub(hc_device_t *hcd)
{
	return hc_setup_virtual_root_hub(hcd, USB_SPEED_HIGH);
}

/** Initialize memory structures used by the EHCI hcd.
 *
 * @param[in] instance EHCI hc driver structure.
 * @return Error code.
 */
errno_t hc_init_memory(hc_t *instance)
{
	assert(instance);
	usb_log_debug2("HC(%p): Initializing Async list(%p).", instance,
	    &instance->async_list);
	errno_t ret = endpoint_list_init(&instance->async_list, "ASYNC");
	if (ret != EOK) {
		usb_log_error("HC(%p): Failed to setup ASYNC list: %s",
		    instance, str_error(ret));
		return ret;
	}
	/* Specs say "Software must set queue head horizontal pointer T-bits to
	 * a zero for queue heads in the asynchronous schedule" (4.4.0).
	 * So we must maintain circular buffer (all horizontal pointers
	 * have to be valid */
	endpoint_list_chain(&instance->async_list, &instance->async_list);

	usb_log_debug2("HC(%p): Initializing Interrupt list (%p).", instance,
	    &instance->int_list);
	ret = endpoint_list_init(&instance->int_list, "INT");
	if (ret != EOK) {
		usb_log_error("HC(%p): Failed to setup INT list: %s",
		    instance, str_error(ret));
		endpoint_list_fini(&instance->async_list);
		return ret;
	}

	/* Take 1024 periodic list heads, we ignore low mem options */
	if (dma_buffer_alloc(&instance->dma_buffer, PAGE_SIZE)) {
		usb_log_error("HC(%p): Failed to get ISO schedule page.",
		    instance);
		endpoint_list_fini(&instance->async_list);
		endpoint_list_fini(&instance->int_list);
		return ENOMEM;
	}
	instance->periodic_list = instance->dma_buffer.virt;

	usb_log_debug2("HC(%p): Initializing Periodic list.", instance);
	for (unsigned i = 0; i < PAGE_SIZE / sizeof(link_pointer_t); ++i) {
		/* Disable everything for now */
		instance->periodic_list[i] =
		    LINK_POINTER_QH(addr_to_phys(instance->int_list.list_head));
	}
	return EOK;
}

/**
 * @}
 */
