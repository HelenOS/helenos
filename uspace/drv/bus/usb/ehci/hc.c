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
#include <str_error.h>
#include <sys/types.h>

#include <usb/debug.h>
#include <usb/usb.h>

//#include "ehci_endpoint.h"
#include "ehci_batch.h"
#include "utils/malloc32.h"

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

static void hc_start(hc_t *instance);
static int hc_init_memory(hc_t *instance);

/** Generate IRQ code.
 * @param[out] ranges PIO ranges buffer.
 * @param[in] hw_res Device's resources.
 *
 * @return Error code.
 */
int ehci_hc_gen_irq_code(irq_code_t *code, const hw_res_list_parsed_t *hw_res)
{
	assert(code);
	assert(hw_res);

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
	ehci_caps_regs_t *caps = NULL;
	int ret = pio_enable_range(&regs, (void**)&caps);
	if (ret != EOK) {
		return ret;
	}
	ehci_regs_t *registers =
	    (ehci_regs_t *)(RNGABSPTR(regs) + EHCI_RD8(caps->caplength));
	code->cmds[0].addr = (void *) &registers->usbsts;
	code->cmds[3].addr = (void *) &registers->usbsts;
	EHCI_WR(code->cmds[1].value, EHCI_USED_INTERRUPTS);

	usb_log_debug("Memory mapped regs at %p (size %zu), IRQ %d.\n",
	    RNGABSPTR(regs), RNGSZ(regs), hw_res->irqs.irqs[0]);

	return hw_res->irqs.irqs[0];
}

/** Initialize EHCI hc driver structure
 *
 * @param[in] instance Memory place for the structure.
 * @param[in] regs Device's I/O registers range.
 * @param[in] interrupts True if w interrupts should be used
 * @return Error code
 */
int hc_init(hc_t *instance, const hw_res_list_parsed_t *hw_res, bool interrupts)
{
	assert(instance);
	assert(hw_res);
	if (hw_res->mem_ranges.count != 1 ||
	    hw_res->mem_ranges.ranges[0].size <
	        (sizeof(ehci_caps_regs_t) + sizeof(ehci_regs_t)))
	    return EINVAL;

	int ret = pio_enable_range(&hw_res->mem_ranges.ranges[0],
	    (void **)&instance->caps);
	if (ret != EOK) {
		usb_log_error("Failed to gain access to device registers: %s.\n",
		    str_error(ret));
		return ret;
	}
	usb_log_info("Device registers at %" PRIx64 " (%zuB) accessible.\n",
	    hw_res->mem_ranges.ranges[0].address.absolute,
	    hw_res->mem_ranges.ranges[0].size);
	instance->registers =
	    (void*)instance->caps + EHCI_RD8(instance->caps->caplength);
	usb_log_info("Device control registers at %" PRIx64 "\n",
	    hw_res->mem_ranges.ranges[0].address.absolute
	    + EHCI_RD8(instance->caps->caplength));

	list_initialize(&instance->pending_batches);
	fibril_mutex_initialize(&instance->guard);
	fibril_condvar_initialize(&instance->async_doorbell);

	ret = hc_init_memory(instance);
	if (ret != EOK) {
		usb_log_error("Failed to create EHCI memory structures: %s.\n",
		    str_error(ret));
		return ret;
	}

	ehci_rh_init(
	    &instance->rh, instance->caps, instance->registers, "ehci rh");
	hc_start(instance);

	return EOK;
}

/** Safely dispose host controller internal structures
 *
 * @param[in] instance Host controller structure to use.
 */
void hc_fini(hc_t *instance)
{
	assert(instance);
	//TODO: stop the hw
#if 0
	endpoint_list_fini(&instance->async_list);
	endpoint_list_fini(&instance->int_list);
	return_page(instance->periodic_list_base);
#endif
};

void hc_enqueue_endpoint(hc_t *instance, const endpoint_t *ep)
{
	assert(instance);
	assert(ep);
	ehci_endpoint_t *ehci_ep = ehci_endpoint_get(ep);
	switch (ep->transfer_type)
	{
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
	switch (ep->transfer_type)
	{
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
	EHCI_SET(instance->registers->usbcmd, USB_CMD_IRQ_ASYNC_DOORBELL);
	fibril_condvar_wait(&instance->async_doorbell, &instance->guard);
	fibril_mutex_unlock(&instance->guard);
}

int ehci_hc_status(hcd_t *hcd, uint32_t *status)
{
	assert(hcd);
	hc_t *instance = hcd->driver.data;
	assert(instance);
	assert(status);
	*status = 0;
	if (instance->registers) {
		*status = EHCI_RD(instance->registers->usbsts);
		EHCI_WR(instance->registers->usbsts, *status);
	}
	return EOK;
}

/** Add USB transfer to the schedule.
 *
 * @param[in] hcd HCD driver structure.
 * @param[in] batch Batch representing the transfer.
 * @return Error code.
 */
int ehci_hc_schedule(hcd_t *hcd, usb_transfer_batch_t *batch)
{
	assert(hcd);
	hc_t *instance = hcd->driver.data;
	assert(instance);

	/* Check for root hub communication */
	if (batch->ep->address == ehci_rh_get_address(&instance->rh)) {
		return ehci_rh_schedule(&instance->rh, batch);
	}
	ehci_transfer_batch_t *ehci_batch = ehci_transfer_batch_get(batch);
	if (!ehci_batch)
		return ENOMEM;

	fibril_mutex_lock(&instance->guard);
	list_append(&ehci_batch->link, &instance->pending_batches);
	ehci_transfer_batch_commit(ehci_batch);

	fibril_mutex_unlock(&instance->guard);
	return EOK;
}

/** Interrupt handling routine
 *
 * @param[in] hcd HCD driver structure.
 * @param[in] status Value of the status register at the time of interrupt.
 */
void ehci_hc_interrupt(hcd_t *hcd, uint32_t status)
{
	assert(hcd);
	hc_t *instance = hcd->driver.data;
	status = EHCI_RD(status);
	assert(instance);

	if (status & USB_STS_PORT_CHANGE_FLAG) {
		ehci_rh_interrupt(&instance->rh);
	}

	if (status & USB_STS_IRQ_ASYNC_ADVANCE_FLAG) {
		fibril_mutex_lock(&instance->guard);
		fibril_condvar_broadcast(&instance->async_doorbell);
		fibril_mutex_unlock(&instance->guard);
	}

	if (status & (USB_STS_IRQ_FLAG | USB_STS_ERR_IRQ_FLAG)) {
		fibril_mutex_lock(&instance->guard);

		link_t *current = list_first(&instance->pending_batches);
		while (current && current != &instance->pending_batches.head) {
			link_t *next = current->next;
			ehci_transfer_batch_t *batch =
			    ehci_transfer_batch_from_link(current);

			if (ehci_transfer_batch_is_complete(batch)) {
				list_remove(current);
				ehci_transfer_batch_finish_dispose(batch);
			}
			current = next;
		}
		fibril_mutex_unlock(&instance->guard);
	}

	if (status & USB_STS_HOST_ERROR_FLAG) {
		usb_log_fatal("HOST CONTROLLER SYSTEM ERROR!\n");
		//TODO do something here
	}
}

/** EHCI hw initialization routine.
 *
 * @param[in] instance EHCI hc driver structure.
 */
void hc_start(hc_t *instance)
{
	assert(instance);
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
		usb_log_info("EHCI turned off.\n");
	} else {
		usb_log_info("EHCI was not running.\n");
	}

	/* Hw initialization sequence, see page 53 (pdf 63) */
	EHCI_SET(instance->registers->usbcmd, USB_CMD_HC_RESET_FLAG);
	while (EHCI_RD(instance->registers->usbcmd) & USB_CMD_HC_RESET_FLAG) {
		async_usleep(1);
	}
	/* Enable interrupts */
	EHCI_WR(instance->registers->usbintr, EHCI_USED_INTERRUPTS);
	/* Use the lowest 4G segment */
	EHCI_WR(instance->registers->ctrldssegment, 0);

	/* Enable periodic list */
	assert(instance->periodic_list_base);
	const uintptr_t phys_base =
	    addr_to_phys((void*)instance->periodic_list_base);
	assert((phys_base & USB_PERIODIC_LIST_BASE_MASK) == phys_base);
	EHCI_WR(instance->registers->periodiclistbase, phys_base);
	EHCI_SET(instance->registers->usbcmd, USB_CMD_PERIODIC_SCHEDULE_FLAG);


	/* Enable Async schedule */
	assert((instance->async_list.list_head_pa & USB_ASYNCLIST_MASK) ==
	    instance->async_list.list_head_pa);
	EHCI_WR(instance->registers->asynclistaddr,
	    instance->async_list.list_head_pa);
	EHCI_SET(instance->registers->usbcmd, USB_CMD_ASYNC_SCHEDULE_FLAG);

	/* Start hc and get all ports */
	EHCI_SET(instance->registers->usbcmd, USB_CMD_RUN_FLAG);
	EHCI_SET(instance->registers->configflag, USB_CONFIG_FLAG_FLAG);

	usb_log_debug("Registers: \n"
	    "\t USBCMD(%p): %x(0x00080000 = at least 1ms between interrupts)\n"
	    "\t USBSTS(%p): %x(0x00001000 = HC halted)\n"
	    "\t USBINT(%p): %x(0x0 = no interrupts).\n"
	    "\t CONFIG(%p): %x(0x0 = ports controlled by companion hc).\n",
	    &instance->registers->usbcmd, EHCI_RD(instance->registers->usbcmd),
	    &instance->registers->usbsts, EHCI_RD(instance->registers->usbsts),
	    &instance->registers->usbintr, EHCI_RD(instance->registers->usbintr),
	    &instance->registers->configflag, EHCI_RD(instance->registers->configflag));
}

/** Initialize memory structures used by the EHCI hcd.
 *
 * @param[in] instance EHCI hc driver structure.
 * @return Error code.
 */
int hc_init_memory(hc_t *instance)
{
	assert(instance);
	int ret = endpoint_list_init(&instance->async_list, "ASYNC");
	if (ret != EOK) {
		usb_log_error("Failed to setup ASYNC list: %s", str_error(ret));
		return ret;
	}

	ret = endpoint_list_init(&instance->int_list, "INT");
	if (ret != EOK) {
		usb_log_error("Failed to setup INT list: %s", str_error(ret));
		endpoint_list_fini(&instance->async_list);
		return ret;
	}
	/* Loop async list */
	endpoint_list_chain(&instance->async_list, &instance->async_list);

	/* Take 1024 periodic list heads, we ignore low mem options */
	instance->periodic_list_base = get_page();
	if (!instance->periodic_list_base) {
		usb_log_error("Failed to get ISO schedule page.");
		endpoint_list_fini(&instance->async_list);
		endpoint_list_fini(&instance->int_list);
		return ENOMEM;
	}
	for (unsigned i = 0;
	    i < PAGE_SIZE/sizeof(instance->periodic_list_base[0]); ++i)
	{
		/* Disable everything for now */
		instance->periodic_list_base[i] =
		    LINK_POINTER_QH(instance->int_list.list_head_pa);
	}
	return EOK;
}

/**
 * @}
 */
