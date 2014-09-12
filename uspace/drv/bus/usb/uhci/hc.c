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
 * @brief UHCI Host controller driver routines
 */
#include <errno.h>
#include <str_error.h>
#include <adt/list.h>
#include <ddi.h>

#include <usb/debug.h>
#include <usb/usb.h>

#include "hc.h"
#include "uhci_batch.h"

#define UHCI_INTR_ALLOW_INTERRUPTS \
    (UHCI_INTR_CRC | UHCI_INTR_COMPLETE | UHCI_INTR_SHORT_PACKET)
#define UHCI_STATUS_USED_INTERRUPTS \
    (UHCI_STATUS_INTERRUPT | UHCI_STATUS_ERROR_INTERRUPT)

static const irq_pio_range_t uhci_irq_pio_ranges[] = {
	{
		.base = 0,
		.size = sizeof(uhci_regs_t)
	}
};

static const irq_cmd_t uhci_irq_commands[] = {
	{
		.cmd = CMD_PIO_READ_16,
		.dstarg = 1,
		.addr = NULL
	},
	{
		.cmd = CMD_AND,
		.srcarg = 1,
		.dstarg = 2,
		.value = UHCI_STATUS_USED_INTERRUPTS | UHCI_STATUS_NM_INTERRUPTS
	},
	{
		.cmd = CMD_PREDICATE,
		.srcarg = 2,
		.value = 2
	},
	{
		.cmd = CMD_PIO_WRITE_A_16,
		.srcarg = 1,
		.addr = NULL
	},
	{
		.cmd = CMD_ACCEPT
	}
};

static void hc_init_hw(const hc_t *instance);
static int hc_init_mem_structures(hc_t *instance);
static int hc_init_transfer_lists(hc_t *instance);
static int hc_schedule(hcd_t *hcd, usb_transfer_batch_t *batch);

static int hc_interrupt_emulator(void *arg);
static int hc_debug_checker(void *arg);

enum {
	/** Number of PIO ranges used in IRQ code */
	hc_irq_pio_range_count =
	    sizeof(uhci_irq_pio_ranges) / sizeof(irq_pio_range_t),

	/* Number of commands used in IRQ code */
	hc_irq_cmd_count =
	    sizeof(uhci_irq_commands) / sizeof(irq_cmd_t)
};

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
	if ((ranges_size < sizeof(uhci_irq_pio_ranges)) ||
	    (cmds_size < sizeof(uhci_irq_commands)) ||
	    (RNGSZ(*regs) < sizeof(uhci_regs_t)))
		return EOVERFLOW;

	memcpy(ranges, uhci_irq_pio_ranges, sizeof(uhci_irq_pio_ranges));
	ranges[0].base = RNGABS(*regs);

	memcpy(cmds, uhci_irq_commands, sizeof(uhci_irq_commands));
	uhci_regs_t *registers = (uhci_regs_t *) RNGABSPTR(*regs);
	cmds[0].addr = (void *) &registers->usbsts;
	cmds[3].addr = (void *) &registers->usbsts;

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
	rc = hc_get_irq_code(irq_ranges, sizeof(irq_ranges), irq_cmds,
	    sizeof(irq_cmds), regs);
	if (rc != EOK) {
		usb_log_error("Failed to generate IRQ commands: %s.\n",
		    str_error(rc));
		return rc;
	}
	
	irq_code_t irq_code = {
		.rangecount = hc_irq_pio_range_count,
		.ranges = irq_ranges,
		.cmdcount = hc_irq_cmd_count,
		.cmds = irq_cmds
	};
	
	/* Register handler to avoid interrupt lockup */
	rc = register_interrupt_handler(device, irq, handler, &irq_code);
	if (rc != EOK) {
		usb_log_error("Failed to register interrupt handler: %s.\n",
		    str_error(rc));
		return rc;
	}
	
	return EOK;
}

/** Take action based on the interrupt cause.
 *
 * @param[in] instance UHCI structure to use.
 * @param[in] status Value of the status register at the time of interrupt.
 *
 * Interrupt might indicate:
 * - transaction completed, either by triggering IOC, SPD, or an error
 * - some kind of device error
 * - resume from suspend state (not implemented)
 */
void hc_interrupt(hc_t *instance, uint16_t status)
{
	assert(instance);
	/* Lower 2 bits are transaction error and transaction complete */
	if (status & (UHCI_STATUS_INTERRUPT | UHCI_STATUS_ERROR_INTERRUPT)) {
		LIST_INITIALIZE(done);
		transfer_list_remove_finished(
		    &instance->transfers_interrupt, &done);
		transfer_list_remove_finished(
		    &instance->transfers_control_slow, &done);
		transfer_list_remove_finished(
		    &instance->transfers_control_full, &done);
		transfer_list_remove_finished(
		    &instance->transfers_bulk_full, &done);

		while (!list_empty(&done)) {
			link_t *item = list_first(&done);
			list_remove(item);
			uhci_transfer_batch_t *batch =
			    uhci_transfer_batch_from_link(item);
			uhci_transfer_batch_finish_dispose(batch);
		}
	}
	/* Resume interrupts are not supported */
	if (status & UHCI_STATUS_RESUME) {
		usb_log_error("Resume interrupt!\n");
	}

	/* Bits 4 and 5 indicate hc error */
	if (status & (UHCI_STATUS_PROCESS_ERROR | UHCI_STATUS_SYSTEM_ERROR)) {
		usb_log_error("UHCI hardware failure!.\n");
		++instance->hw_failures;
		transfer_list_abort_all(&instance->transfers_interrupt);
		transfer_list_abort_all(&instance->transfers_control_slow);
		transfer_list_abort_all(&instance->transfers_control_full);
		transfer_list_abort_all(&instance->transfers_bulk_full);

		if (instance->hw_failures < UHCI_ALLOWED_HW_FAIL) {
			/* reinitialize hw, this triggers virtual disconnect*/
			hc_init_hw(instance);
		} else {
			usb_log_fatal("Too many UHCI hardware failures!.\n");
			hc_fini(instance);
		}
	}
}

/** Initialize UHCI hc driver structure
 *
 * @param[in] instance Memory place to initialize.
 * @param[in] HC function node
 * @param[in] regs Range of device's I/O control registers.
 * @param[in] interrupts True if hw interrupts should be used.
 * @return Error code.
 * @note Should be called only once on any structure.
 *
 * Initializes memory structures, starts up hw, and launches debugger and
 * interrupt fibrils.
 */
int hc_init(hc_t *instance, ddf_fun_t *fun, addr_range_t *regs, bool interrupts)
{
	assert(regs->size >= sizeof(uhci_regs_t));
	int rc;

	instance->hw_interrupts = interrupts;
	instance->hw_failures = 0;

	/* allow access to hc control registers */
	uhci_regs_t *io;
	rc = pio_enable_range(regs, (void **) &io);
	if (rc != EOK) {
		usb_log_error("Failed to gain access to registers at %p: %s.\n",
		    io, str_error(rc));
		return rc;
	}

	instance->registers = io;
	usb_log_debug(
	    "Device registers at %p (%zuB) accessible.\n", io, regs->size);

	rc = hc_init_mem_structures(instance);
	if (rc != EOK) {
		usb_log_error("Failed to initialize UHCI memory structures: %s.\n",
		    str_error(rc));
		return rc;
	}

	instance->generic = ddf_fun_data_alloc(fun, sizeof(hcd_t));
	if (instance->generic == NULL) {
		usb_log_error("Out of memory.\n");
		return ENOMEM;
	}

	hcd_init(instance->generic, USB_SPEED_FULL,
	    BANDWIDTH_AVAILABLE_USB11, bandwidth_count_usb11);

	instance->generic->private_data = instance;
	instance->generic->schedule = hc_schedule;
	instance->generic->ep_add_hook = NULL;

	hc_init_hw(instance);
	if (!interrupts) {
		instance->interrupt_emulator =
		    fibril_create(hc_interrupt_emulator, instance);
		fibril_add_ready(instance->interrupt_emulator);
	}
	(void)hc_debug_checker;

	return EOK;
}

/** Initialize UHCI hc hw resources.
 *
 * @param[in] instance UHCI structure to use.
 * For magic values see UHCI Design Guide
 */
void hc_init_hw(const hc_t *instance)
{
	assert(instance);
	uhci_regs_t *registers = instance->registers;

	/* Reset everything, who knows what touched it before us */
	pio_write_16(&registers->usbcmd, UHCI_CMD_GLOBAL_RESET);
	async_usleep(50000); /* 50ms according to USB spec(root hub reset) */
	pio_write_16(&registers->usbcmd, 0);

	/* Reset hc, all states and counters. Hope that hw is not broken */
	pio_write_16(&registers->usbcmd, UHCI_CMD_HCRESET);
	do { async_usleep(10); }
	while ((pio_read_16(&registers->usbcmd) & UHCI_CMD_HCRESET) != 0);

	/* Set frame to exactly 1ms */
	pio_write_8(&registers->sofmod, 64);

	/* Set frame list pointer */
	const uint32_t pa = addr_to_phys(instance->frame_list);
	pio_write_32(&registers->flbaseadd, pa);

	if (instance->hw_interrupts) {
		/* Enable all interrupts, but resume interrupt */
		pio_write_16(&instance->registers->usbintr,
		    UHCI_INTR_ALLOW_INTERRUPTS);
	}

	const uint16_t cmd = pio_read_16(&registers->usbcmd);
	if (cmd != 0)
		usb_log_warning("Previous command value: %x.\n", cmd);

	/* Start the hc with large(64B) packet FSBR */
	pio_write_16(&registers->usbcmd,
	    UHCI_CMD_RUN_STOP | UHCI_CMD_MAX_PACKET | UHCI_CMD_CONFIGURE);
}

/** Initialize UHCI hc memory structures.
 *
 * @param[in] instance UHCI structure to use.
 * @return Error code
 * @note Should be called only once on any structure.
 *
 * Structures:
 *  - transfer lists (queue heads need to be accessible by the hw)
 *  - frame list page (needs to be one UHCI hw accessible 4K page)
 */
int hc_init_mem_structures(hc_t *instance)
{
	assert(instance);

	/* Init USB frame list page */
	instance->frame_list = get_page();
	if (!instance->frame_list) {
		return ENOMEM;
	}
	usb_log_debug("Initialized frame list at %p.\n", instance->frame_list);

	/* Init transfer lists */
	int ret = hc_init_transfer_lists(instance);
	if (ret != EOK) {
		usb_log_error("Failed to initialize transfer lists.\n");
		return_page(instance->frame_list);
		return ENOMEM;
	}
	usb_log_debug("Initialized transfer lists.\n");


	/* Set all frames to point to the first queue head */
	const uint32_t queue = LINK_POINTER_QH(
	        addr_to_phys(instance->transfers_interrupt.queue_head));

	for (unsigned i = 0; i < UHCI_FRAME_LIST_COUNT; ++i) {
		instance->frame_list[i] = queue;
	}

	return EOK;
}

/** Initialize UHCI hc transfer lists.
 *
 * @param[in] instance UHCI structure to use.
 * @return Error code
 * @note Should be called only once on any structure.
 *
 * Initializes transfer lists and sets them in one chain to support proper
 * USB scheduling. Sets pointer table for quick access.
 */
int hc_init_transfer_lists(hc_t *instance)
{
	assert(instance);
#define SETUP_TRANSFER_LIST(type, name) \
do { \
	int ret = transfer_list_init(&instance->transfers_##type, name); \
	if (ret != EOK) { \
		usb_log_error("Failed to setup %s transfer list: %s.\n", \
		    name, str_error(ret)); \
		transfer_list_fini(&instance->transfers_bulk_full); \
		transfer_list_fini(&instance->transfers_control_full); \
		transfer_list_fini(&instance->transfers_control_slow); \
		transfer_list_fini(&instance->transfers_interrupt); \
		return ret; \
	} \
} while (0)

	SETUP_TRANSFER_LIST(bulk_full, "BULK FULL");
	SETUP_TRANSFER_LIST(control_full, "CONTROL FULL");
	SETUP_TRANSFER_LIST(control_slow, "CONTROL LOW");
	SETUP_TRANSFER_LIST(interrupt, "INTERRUPT");
#undef SETUP_TRANSFER_LIST
	/* Connect lists into one schedule */
	transfer_list_set_next(&instance->transfers_control_full,
		&instance->transfers_bulk_full);
	transfer_list_set_next(&instance->transfers_control_slow,
		&instance->transfers_control_full);
	transfer_list_set_next(&instance->transfers_interrupt,
		&instance->transfers_control_slow);

	/*FSBR, This feature is not needed (adds no benefit) and is supposedly
	 * buggy on certain hw, enable at your own risk. */
#ifdef FSBR
	transfer_list_set_next(&instance->transfers_bulk_full,
	    &instance->transfers_control_full);
#endif

	/* Assign pointers to be used during scheduling */
	instance->transfers[USB_SPEED_FULL][USB_TRANSFER_INTERRUPT] =
	  &instance->transfers_interrupt;
	instance->transfers[USB_SPEED_LOW][USB_TRANSFER_INTERRUPT] =
	  &instance->transfers_interrupt;
	instance->transfers[USB_SPEED_FULL][USB_TRANSFER_CONTROL] =
	  &instance->transfers_control_full;
	instance->transfers[USB_SPEED_LOW][USB_TRANSFER_CONTROL] =
	  &instance->transfers_control_slow;
	instance->transfers[USB_SPEED_FULL][USB_TRANSFER_BULK] =
	  &instance->transfers_bulk_full;
	instance->transfers[USB_SPEED_LOW][USB_TRANSFER_BULK] =
	  &instance->transfers_bulk_full;

	return EOK;
}

/** Schedule batch for execution.
 *
 * @param[in] instance UHCI structure to use.
 * @param[in] batch Transfer batch to schedule.
 * @return Error code
 *
 * Checks for bandwidth availability and appends the batch to the proper queue.
 */
int hc_schedule(hcd_t *hcd, usb_transfer_batch_t *batch)
{
	assert(hcd);
	hc_t *instance = hcd->private_data;
	assert(instance);
	assert(batch);
	uhci_transfer_batch_t *uhci_batch = uhci_transfer_batch_get(batch);
	if (!uhci_batch) {
		usb_log_error("Failed to create UHCI transfer structures.\n");
		return ENOMEM;
	}

	transfer_list_t *list =
	    instance->transfers[batch->ep->speed][batch->ep->transfer_type];
	assert(list);
	transfer_list_add_batch(list, uhci_batch);

	return EOK;
}

/** Polling function, emulates interrupts.
 *
 * @param[in] arg UHCI hc structure to use.
 * @return EOK (should never return)
 */
int hc_interrupt_emulator(void* arg)
{
	usb_log_debug("Started interrupt emulator.\n");
	hc_t *instance = arg;
	assert(instance);

	while (1) {
		/* Read and clear status register */
		uint16_t status = pio_read_16(&instance->registers->usbsts);
		pio_write_16(&instance->registers->usbsts, status);
		if (status != 0)
			usb_log_debug2("UHCI status: %x.\n", status);
		hc_interrupt(instance, status);
		async_usleep(UHCI_INT_EMULATOR_TIMEOUT);
	}
	return EOK;
}

/** Debug function, checks consistency of memory structures.
 *
 * @param[in] arg UHCI structure to use.
 * @return EOK (should never return)
 */
int hc_debug_checker(void *arg)
{
	hc_t *instance = arg;
	assert(instance);

#define QH(queue) \
	instance->transfers_##queue.queue_head

	while (1) {
		const uint16_t cmd = pio_read_16(&instance->registers->usbcmd);
		const uint16_t sts = pio_read_16(&instance->registers->usbsts);
		const uint16_t intr =
		    pio_read_16(&instance->registers->usbintr);

		if (((cmd & UHCI_CMD_RUN_STOP) != 1) || (sts != 0)) {
			usb_log_debug2("Command: %X Status: %X Intr: %x\n",
			    cmd, sts, intr);
		}

		const uintptr_t frame_list =
		    pio_read_32(&instance->registers->flbaseadd) & ~0xfff;
		if (frame_list != addr_to_phys(instance->frame_list)) {
			usb_log_debug("Framelist address: %p vs. %p.\n",
			    (void *) frame_list,
			    (void *) addr_to_phys(instance->frame_list));
		}

		int frnum = pio_read_16(&instance->registers->frnum) & 0x3ff;

		uintptr_t expected_pa = instance->frame_list[frnum]
		    & LINK_POINTER_ADDRESS_MASK;
		uintptr_t real_pa = addr_to_phys(QH(interrupt));
		if (expected_pa != real_pa) {
			usb_log_debug("Interrupt QH: %p (frame %d) vs. %p.\n",
			    (void *) expected_pa, frnum, (void *) real_pa);
		}

		expected_pa = QH(interrupt)->next & LINK_POINTER_ADDRESS_MASK;
		real_pa = addr_to_phys(QH(control_slow));
		if (expected_pa != real_pa) {
			usb_log_debug("Control Slow QH: %p vs. %p.\n",
			    (void *) expected_pa, (void *) real_pa);
		}

		expected_pa = QH(control_slow)->next & LINK_POINTER_ADDRESS_MASK;
		real_pa = addr_to_phys(QH(control_full));
		if (expected_pa != real_pa) {
			usb_log_debug("Control Full QH: %p vs. %p.\n",
			    (void *) expected_pa, (void *) real_pa);
		}

		expected_pa = QH(control_full)->next & LINK_POINTER_ADDRESS_MASK;
		real_pa = addr_to_phys(QH(bulk_full));
		if (expected_pa != real_pa ) {
			usb_log_debug("Bulk QH: %p vs. %p.\n",
			    (void *) expected_pa, (void *) real_pa);
		}
		async_usleep(UHCI_DEBUGER_TIMEOUT);
	}
	return EOK;
#undef QH
}
/**
 * @}
 */
