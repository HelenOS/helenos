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

/** @addtogroup drvusbohcihc
 * @{
 */
/** @file
 * @brief OHCI Host controller driver routines
 */

#include <errno.h>
#include <stdbool.h>
#include <str_error.h>
#include <adt/list.h>
#include <libarch/ddi.h>

#include <usb/debug.h>
#include <usb/usb.h>
#include <usb/ddfiface.h>

#include "hc.h"
#include "ohci_endpoint.h"

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

enum {
	/** Number of PIO ranges used in IRQ code */
	hc_irq_pio_range_count = 
	    sizeof(ohci_pio_ranges) / sizeof(irq_pio_range_t),

	/** Number of commands used in IRQ code */
	hc_irq_cmd_count =
	    sizeof(ohci_irq_commands) / sizeof(irq_cmd_t)
};

static void hc_gain_control(hc_t *instance);
static void hc_start(hc_t *instance);
static int hc_init_transfer_lists(hc_t *instance);
static int hc_init_memory(hc_t *instance);
static int interrupt_emulator(hc_t *instance);
static int hc_schedule(hcd_t *hcd, usb_transfer_batch_t *batch);

/** Generate IRQ code.
 * @param[out] ranges PIO ranges buffer.
 * @param[in] ranges_size Size of the ranges buffer (bytes).
 * @param[out] cmds Commands buffer.
 * @param[in] cmds_size Size of the commands buffer (bytes).
 * @param[in] regs Device's register range.
 *
 * @return Error code.
 */
int
hc_get_irq_code(irq_pio_range_t ranges[], size_t ranges_size, irq_cmd_t cmds[],
    size_t cmds_size, addr_range_t *regs)
{
	if ((ranges_size < sizeof(ohci_pio_ranges)) ||
	    (cmds_size < sizeof(ohci_irq_commands)) ||
	    (RNGSZ(*regs) < sizeof(ohci_regs_t)))
		return EOVERFLOW;

	memcpy(ranges, ohci_pio_ranges, sizeof(ohci_pio_ranges));
	ranges[0].base = RNGABS(*regs);

	memcpy(cmds, ohci_irq_commands, sizeof(ohci_irq_commands));
	ohci_regs_t *registers = (ohci_regs_t *) RNGABSPTR(*regs);
	cmds[0].addr = (void *) &registers->interrupt_status;
	cmds[3].addr = (void *) &registers->interrupt_status;
	OHCI_WR(cmds[1].value, OHCI_USED_INTERRUPTS);

	return EOK;
}

/** Register interrupt handler.
 *
 * @param[in] device Host controller DDF device
 * @param[in] regs Register range
 * @param[in] irq Interrupt number
 * @paran[in] handler Interrupt handler
 *
 * @return EOK on success or negative error code
 */
int hc_register_irq_handler(ddf_dev_t *device, addr_range_t *regs, int irq,
    interrupt_handler_t handler)
{
	int rc;

	irq_pio_range_t irq_ranges[hc_irq_pio_range_count];
	irq_cmd_t irq_cmds[hc_irq_cmd_count];

	irq_code_t irq_code = {
		.rangecount = hc_irq_pio_range_count,
		.ranges = irq_ranges,
		.cmdcount = hc_irq_cmd_count,
		.cmds = irq_cmds
	};

	rc = hc_get_irq_code(irq_ranges, sizeof(irq_ranges), irq_cmds,
	    sizeof(irq_cmds), regs);
	if (rc != EOK) {
		usb_log_error("Failed to generate IRQ code: %s.\n",
		    str_error(rc));
		return rc;
	}

	/* Register handler to avoid interrupt lockup */
	rc = register_interrupt_handler(device, irq, handler, &irq_code);
	if (rc != EOK) {
		usb_log_error("Failed to register interrupt handler: %s.\n",
		    str_error(rc));
		return rc;
	}

	return EOK;
}

/** Announce OHCI root hub to the DDF
 *
 * @param[in] instance OHCI driver intance
 * @param[in] hub_fun DDF fuction representing OHCI root hub
 * @return Error code
 */
int hc_register_hub(hc_t *instance, ddf_fun_t *hub_fun)
{
	bool addr_reqd = false;
	bool ep_added = false;
	bool fun_bound = false;
	int rc;

	assert(instance);
	assert(hub_fun);

	/* Try to get address 1 for root hub. */
	instance->rh.address = 1;
	rc = usb_device_manager_request_address(
	    &instance->generic->dev_manager, &instance->rh.address, false,
	    USB_SPEED_FULL);
	if (rc != EOK) {
		usb_log_error("Failed to get OHCI root hub address: %s\n",
		    str_error(rc));
		goto error;
	}

	addr_reqd = true;

	rc = usb_endpoint_manager_add_ep(
	    &instance->generic->ep_manager, instance->rh.address, 0,
	    USB_DIRECTION_BOTH, USB_TRANSFER_CONTROL, USB_SPEED_FULL, 64,
	    0, NULL, NULL);
	if (rc != EOK) {
    	        usb_log_error("Failed to register root hub control endpoint: %s.\n",
		    str_error(rc));
		goto error;
	}

	ep_added = true;

	rc = ddf_fun_add_match_id(hub_fun, "usb&class=hub", 100);
	if (rc != EOK) {
		usb_log_error("Failed to add root hub match-id: %s.\n",
		    str_error(rc));
		goto error;
	}

	rc = ddf_fun_bind(hub_fun);
	if (rc != EOK) {
		usb_log_error("Failed to bind root hub function: %s.\n",
		    str_error(rc));
		goto error;
	}

	fun_bound = true;

	rc = usb_device_manager_bind_address(&instance->generic->dev_manager,
	    instance->rh.address, ddf_fun_get_handle(hub_fun));
	if (rc != EOK) {
		usb_log_warning("Failed to bind root hub address: %s.\n",
		    str_error(rc));
	}

	return EOK;
error:
	if (fun_bound)
		ddf_fun_unbind(hub_fun);
	if (ep_added) {
		usb_endpoint_manager_remove_ep(
		    &instance->generic->ep_manager, instance->rh.address, 0,
		    USB_DIRECTION_BOTH, NULL, NULL);
	}
	if (addr_reqd) {
		usb_device_manager_release_address(
		    &instance->generic->dev_manager, instance->rh.address);
	}
	return rc;
}

/** Initialize OHCI hc driver structure
 *
 * @param[in] instance Memory place for the structure.
 * @param[in] HC function node
 * @param[in] regs Device's I/O registers range.
 * @param[in] interrupts True if w interrupts should be used
 * @return Error code
 */
int hc_init(hc_t *instance, ddf_fun_t *fun, addr_range_t *regs, bool interrupts)
{
	assert(instance);

	int rc = pio_enable_range(regs, (void **) &instance->registers);
	if (rc != EOK) {
		usb_log_error("Failed to gain access to device registers: %s.\n",
		    str_error(rc));
		return rc;
	}

	list_initialize(&instance->pending_batches);

	instance->generic = ddf_fun_data_alloc(fun, sizeof(hcd_t));
	if (instance->generic == NULL) {
		usb_log_error("Out of memory.\n");
		return ENOMEM;
	}

	hcd_init(instance->generic, USB_SPEED_FULL,
	    BANDWIDTH_AVAILABLE_USB11, bandwidth_count_usb11);
	instance->generic->private_data = instance;
	instance->generic->schedule = hc_schedule;
	instance->generic->ep_add_hook = ohci_endpoint_init;
	instance->generic->ep_remove_hook = ohci_endpoint_fini;

	rc = hc_init_memory(instance);
	if (rc != EOK) {
		usb_log_error("Failed to create OHCI memory structures: %s.\n",
		    str_error(rc));
		return rc;
	}

	fibril_mutex_initialize(&instance->guard);

	hc_gain_control(instance);

	if (!interrupts) {
		instance->interrupt_emulator =
		    fibril_create((int(*)(void*))interrupt_emulator, instance);
		fibril_add_ready(instance->interrupt_emulator);
	}

	rh_init(&instance->rh, instance->registers);
	hc_start(instance);

	return EOK;
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

/** Add USB transfer to the schedule.
 *
 * @param[in] instance OHCI hc driver structure.
 * @param[in] batch Batch representing the transfer.
 * @return Error code.
 */
int hc_schedule(hcd_t *hcd, usb_transfer_batch_t *batch)
{
	assert(hcd);
	hc_t *instance = hcd->private_data;
	assert(instance);

	/* Check for root hub communication */
	if (batch->ep->address == instance->rh.address) {
		usb_log_debug("OHCI root hub request.\n");
		rh_request(&instance->rh, batch);
		return EOK;
	}
	ohci_transfer_batch_t *ohci_batch = ohci_transfer_batch_get(batch);
	if (!ohci_batch)
		return ENOMEM;

	fibril_mutex_lock(&instance->guard);
	list_append(&ohci_batch->link, &instance->pending_batches);
	ohci_transfer_batch_commit(ohci_batch);

	/* Control and bulk schedules need a kick to start working */
	switch (batch->ep->transfer_type)
	{
	case USB_TRANSFER_CONTROL:
		OHCI_SET(instance->registers->command_status, CS_CLF);
		break;
	case USB_TRANSFER_BULK:
		OHCI_SET(instance->registers->command_status, CS_BLF);
		break;
	default:
		break;
	}
	fibril_mutex_unlock(&instance->guard);
	return EOK;
}

/** Interrupt handling routine
 *
 * @param[in] instance OHCI hc driver structure.
 * @param[in] status Value of the status register at the time of interrupt.
 */
void hc_interrupt(hc_t *instance, uint32_t status)
{
	status = OHCI_RD(status);
	assert(instance);
	if ((status & ~I_SF) == 0) /* ignore sof status */
		return;
	usb_log_debug2("OHCI(%p) interrupt: %x.\n", instance, status);
	if (status & I_RHSC)
		rh_interrupt(&instance->rh);

	if (status & I_WDH) {
		fibril_mutex_lock(&instance->guard);
		usb_log_debug2("HCCA: %p-%#" PRIx32 " (%p).\n", instance->hcca,
		    OHCI_RD(instance->registers->hcca),
		    (void *) addr_to_phys(instance->hcca));
		usb_log_debug2("Periodic current: %#" PRIx32 ".\n",
		    OHCI_RD(instance->registers->periodic_current));

		link_t *current = list_first(&instance->pending_batches);
		while (current && current != &instance->pending_batches.head) {
			link_t *next = current->next;
			ohci_transfer_batch_t *batch =
			    ohci_transfer_batch_from_link(current);

			if (ohci_transfer_batch_is_complete(batch)) {
				list_remove(current);
				ohci_transfer_batch_finish_dispose(batch);
			}

			current = next;
		}
		fibril_mutex_unlock(&instance->guard);
	}

	if (status & I_UE) {
		usb_log_fatal("Error like no other!\n");
		hc_start(instance);
	}

}

/** Check status register regularly
 *
 * @param[in] instance OHCI hc driver structure.
 * @return Error code
 */
int interrupt_emulator(hc_t *instance)
{
	assert(instance);
	usb_log_info("Started interrupt emulator.\n");
	while (1) {
		const uint32_t status = instance->registers->interrupt_status;
		instance->registers->interrupt_status = status;
		hc_interrupt(instance, status);
		async_usleep(10000);
	}
	return EOK;
}

/** Turn off any (BIOS)driver that might be in control of the device.
 *
 * This function implements routines described in chapter 5.1.1.3 of the OHCI
 * specification (page 40, pdf page 54).
 *
 * @param[in] instance OHCI hc driver structure.
 */
void hc_gain_control(hc_t *instance)
{
	assert(instance);

	usb_log_debug("Requesting OHCI control.\n");
	if (OHCI_RD(instance->registers->revision) & R_LEGACY_FLAG) {
		/* Turn off legacy emulation, it should be enough to zero
		 * the lowest bit, but it caused problems. Thus clear all
		 * except GateA20 (causes restart on some hw).
		 * See page 145 of the specs for details.
		 */
		volatile uint32_t *ohci_emulation_reg =
		(uint32_t*)((char*)instance->registers + LEGACY_REGS_OFFSET);
		usb_log_debug("OHCI legacy register %p: %x.\n",
		    ohci_emulation_reg, OHCI_RD(*ohci_emulation_reg));
		/* Zero everything but A20State */
		OHCI_CLR(*ohci_emulation_reg, ~0x100);
		usb_log_debug(
		    "OHCI legacy register (should be 0 or 0x100) %p: %x.\n",
		    ohci_emulation_reg, OHCI_RD(*ohci_emulation_reg));
	}

	/* Interrupt routing enabled => smm driver is active */
	if (OHCI_RD(instance->registers->control) & C_IR) {
		usb_log_debug("SMM driver: request ownership change.\n");
		OHCI_SET(instance->registers->command_status, CS_OCR);
		/* Hope that SMM actually knows its stuff or we can hang here */
		while (OHCI_RD(instance->registers->control & C_IR)) {
			async_usleep(1000);
		}
		usb_log_info("SMM driver: Ownership taken.\n");
		C_HCFS_SET(instance->registers->control, C_HCFS_RESET);
		async_usleep(50000);
		return;
	}

	const unsigned hc_status = C_HCFS_GET(instance->registers->control);
	/* Interrupt routing disabled && status != USB_RESET => BIOS active */
	if (hc_status != C_HCFS_RESET) {
		usb_log_debug("BIOS driver found.\n");
		if (hc_status == C_HCFS_OPERATIONAL) {
			usb_log_info("BIOS driver: HC operational.\n");
			return;
		}
		/* HC is suspended assert resume for 20ms */
		C_HCFS_SET(instance->registers->control, C_HCFS_RESUME);
		async_usleep(20000);
		usb_log_info("BIOS driver: HC resumed.\n");
		return;
	}

	/* HC is in reset (hw startup) => no other driver
	 * maintain reset for at least the time specified in USB spec (50 ms)*/
	usb_log_debug("Host controller found in reset state.\n");
	async_usleep(50000);
}

/** OHCI hw initialization routine.
 *
 * @param[in] instance OHCI hc driver structure.
 */
void hc_start(hc_t *instance)
{
	/* OHCI guide page 42 */
	assert(instance);
	usb_log_debug2("Started hc initialization routine.\n");

	/* Save contents of fm_interval register */
	const uint32_t fm_interval = OHCI_RD(instance->registers->fm_interval);
	usb_log_debug2("Old value of HcFmInterval: %x.\n", fm_interval);

	/* Reset hc */
	usb_log_debug2("HC reset.\n");
	size_t time = 0;
	OHCI_WR(instance->registers->command_status, CS_HCR);
	while (OHCI_RD(instance->registers->command_status) & CS_HCR) {
		async_usleep(10);
		time += 10;
	}
	usb_log_debug2("HC reset complete in %zu us.\n", time);

	/* Restore fm_interval */
	OHCI_WR(instance->registers->fm_interval, fm_interval);
	assert((OHCI_RD(instance->registers->command_status) & CS_HCR) == 0);

	/* hc is now in suspend state */
	usb_log_debug2("HC should be in suspend state(%x).\n",
	    OHCI_RD(instance->registers->control));

	/* Use HCCA */
	OHCI_WR(instance->registers->hcca, addr_to_phys(instance->hcca));

	/* Use queues */
	OHCI_WR(instance->registers->bulk_head,
	    instance->lists[USB_TRANSFER_BULK].list_head_pa);
	usb_log_debug2("Bulk HEAD set to: %p (%#" PRIx32 ").\n",
	    instance->lists[USB_TRANSFER_BULK].list_head,
	    instance->lists[USB_TRANSFER_BULK].list_head_pa);

	OHCI_WR(instance->registers->control_head,
	    instance->lists[USB_TRANSFER_CONTROL].list_head_pa);
	usb_log_debug2("Control HEAD set to: %p (%#" PRIx32 ").\n",
	    instance->lists[USB_TRANSFER_CONTROL].list_head,
	    instance->lists[USB_TRANSFER_CONTROL].list_head_pa);

	/* Enable queues */
	OHCI_SET(instance->registers->control, (C_PLE | C_IE | C_CLE | C_BLE));
	usb_log_debug("Queues enabled(%x).\n",
	    OHCI_RD(instance->registers->control));

	/* Enable interrupts */
	OHCI_WR(instance->registers->interrupt_enable, OHCI_USED_INTERRUPTS);
	usb_log_debug("Enabled interrupts: %x.\n",
	    OHCI_RD(instance->registers->interrupt_enable));
	OHCI_WR(instance->registers->interrupt_enable, I_MI);

	/* Set periodic start to 90% */
	const uint32_t frame_length =
	    (fm_interval >> FMI_FI_SHIFT) & FMI_FI_MASK;
	OHCI_WR(instance->registers->periodic_start,
	    ((frame_length / 10) * 9) & PS_MASK << PS_SHIFT);
	usb_log_debug2("All periodic start set to: %x(%u - 90%% of %d).\n",
	    OHCI_RD(instance->registers->periodic_start),
	    OHCI_RD(instance->registers->periodic_start), frame_length);
	C_HCFS_SET(instance->registers->control, C_HCFS_OPERATIONAL);
	usb_log_debug("OHCI HC up and running (ctl_reg=0x%x).\n",
	    OHCI_RD(instance->registers->control));
}

/** Initialize schedule queues
 *
 * @param[in] instance OHCI hc driver structure
 * @return Error code
 */
int hc_init_transfer_lists(hc_t *instance)
{
	assert(instance);
#define SETUP_ENDPOINT_LIST(type) \
do { \
	const char *name = usb_str_transfer_type(type); \
	int ret = endpoint_list_init(&instance->lists[type], name); \
	if (ret != EOK) { \
		usb_log_error("Failed to setup %s endpoint list: %s.\n", \
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
int hc_init_memory(hc_t *instance)
{
	assert(instance);

	memset(&instance->rh, 0, sizeof(instance->rh));
	/* Init queues */
	const int ret = hc_init_transfer_lists(instance);
	if (ret != EOK) {
		return ret;
	}

	/*Init HCCA */
	instance->hcca = hcca_get();
	if (instance->hcca == NULL)
		return ENOMEM;
	usb_log_debug2("OHCI HCCA initialized at %p.\n", instance->hcca);

	for (unsigned i = 0; i < HCCA_INT_EP_COUNT; ++i) {
		hcca_set_int_ep(instance->hcca, i,
		    instance->lists[USB_TRANSFER_INTERRUPT].list_head_pa);
	}
	usb_log_debug2("Interrupt HEADs set to: %p (%#" PRIx32 ").\n",
	    instance->lists[USB_TRANSFER_INTERRUPT].list_head,
	    instance->lists[USB_TRANSFER_INTERRUPT].list_head_pa);

	return EOK;
}

/**
 * @}
 */
