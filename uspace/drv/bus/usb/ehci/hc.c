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
//#include "ehci_batch.h"
#include "utils/malloc32.h"

#include "hc.h"

#define EHCI_USED_INTERRUPTS \
    (USB_INTR_IRQ_FLAG | USB_INTR_ERR_IRQ_FLAG | USB_INTR_PORT_CHANGE_FLAG)

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

static void hc_gain_control(hc_t *instance);
static void hc_start(hc_t *instance);
static int hc_init_memory(hc_t *instance);

/** Generate IRQ code.
 * @param[out] ranges PIO ranges buffer.
 * @param[in] ranges_size Size of the ranges buffer (bytes).
 * @param[out] cmds Commands buffer.
 * @param[in] cmds_size Size of the commands buffer (bytes).
 * @param[in] hw_res Device's resources.
 *
 * @return Error code.
 */
int hc_gen_irq_code(irq_code_t *code, const hw_res_list_parsed_t *hw_res)
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
	usb_log_debug("Device registers at %" PRIx64 " (%zuB) accessible.\n",
	    hw_res->mem_ranges.ranges[0].address.absolute,
	    hw_res->mem_ranges.ranges[0].size);
	instance->registers =
	    (void*)instance->caps + EHCI_RD8(instance->caps->caplength);

	list_initialize(&instance->pending_batches);
	fibril_mutex_initialize(&instance->guard);

	ret = hc_init_memory(instance);
	if (ret != EOK) {
		usb_log_error("Failed to create EHCI memory structures: %s.\n",
		    str_error(ret));
		return ret;
	}

	hc_gain_control(instance);

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
	/* TODO: implement*/
};

void hc_enqueue_endpoint(hc_t *instance, const endpoint_t *ep)
{
}

void hc_dequeue_endpoint(hc_t *instance, const endpoint_t *ep)
{
}

/** Add USB transfer to the schedule.
 *
 * @param[in] instance EHCI hc driver structure.
 * @param[in] batch Batch representing the transfer.
 * @return Error code.
 */
int hc_schedule(hcd_t *hcd, usb_transfer_batch_t *batch)
{
	assert(hcd);
	hc_t *instance = hcd->driver.data;
	assert(instance);

	/* Check for root hub communication */
	if (batch->ep->address == ehci_rh_get_address(&instance->rh)) {
		usb_log_debug("EHCI root hub request.\n");
		return ehci_rh_schedule(&instance->rh, batch);
	}
	return ENOTSUP;
}

/** Interrupt handling routine
 *
 * @param[in] instance EHCI hc driver structure.
 * @param[in] status Value of the status register at the time of interrupt.
 */
void hc_interrupt(hcd_t *hcd, uint32_t status)
{
	assert(hcd);
	hc_t *instance = hcd->driver.data;
	status = EHCI_RD(status);
	assert(instance);
	if (status & USB_STS_PORT_CHANGE_FLAG) {
		ehci_rh_interrupt(&instance->rh);
	}
}

/** Turn off any (BIOS)driver that might be in control of the device.
 *
 * This function implements routines described in chapter 5.1.1.3 of the EHCI
 * specification (page 40, pdf page 54).
 *
 * @param[in] instance EHCI hc driver structure.
 */
void hc_gain_control(hc_t *instance)
{
	assert(instance);
}

/** EHCI hw initialization routine.
 *
 * @param[in] instance EHCI hc driver structure.
 */
void hc_start(hc_t *instance)
{
}

/** Initialize memory structures used by the EHCI hcd.
 *
 * @param[in] instance EHCI hc driver structure.
 * @return Error code.
 */
int hc_init_memory(hc_t *instance)
{
	return EOK;
}

/**
 * @}
 */
