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
#include <libarch/ddi.h>

#include <usb/debug.h>
#include <usb/usb.h>
#include <usb/ddfiface.h>
#include <usb_iface.h>

#include "hc.h"

static irq_cmd_t uhci_cmds[] = {
	{
		.cmd = CMD_PIO_READ_16,
		.addr = NULL, /* patched for every instance */
		.dstarg = 1
	},
	{
		.cmd = CMD_PIO_WRITE_16,
		.addr = NULL, /* pathed for every instance */
		.value = 0x1f
	},
	{
		.cmd = CMD_ACCEPT
	}
};
/*----------------------------------------------------------------------------*/
static int hc_init_transfer_lists(hc_t *instance);
static int hc_init_mem_structures(hc_t *instance);
static void hc_init_hw(hc_t *instance);

static int hc_interrupt_emulator(void *arg);
static int hc_debug_checker(void *arg);

static bool usb_is_allowed(
    bool low_speed, usb_transfer_type_t transfer, size_t size);
/*----------------------------------------------------------------------------*/
/** Initialize UHCI hcd driver structure
 *
 * @param[in] instance Memory place to initialize.
 * @param[in] fun DDF function.
 * @param[in] regs Address of I/O control registers.
 * @param[in] size Size of I/O control registers.
 * @return Error code.
 * @note Should be called only once on any structure.
 *
 * Initializes memory structures, starts up hw, and launches debugger and
 * interrupt fibrils.
 */
int hc_init(hc_t *instance, ddf_fun_t *fun,
    void *regs, size_t reg_size, bool interrupts)
{
	assert(reg_size >= sizeof(regs_t));
	int ret;

#define CHECK_RET_DEST_FUN_RETURN(ret, message...) \
	if (ret != EOK) { \
		usb_log_error(message); \
		if (instance->ddf_instance) \
			ddf_fun_destroy(instance->ddf_instance); \
		return ret; \
	} else (void) 0

	instance->hw_interrupts = interrupts;
	instance->hw_failures = 0;

	/* Setup UHCI function. */
	instance->ddf_instance = fun;

	/* allow access to hc control registers */
	regs_t *io;
	ret = pio_enable(regs, reg_size, (void**)&io);
	CHECK_RET_DEST_FUN_RETURN(ret,
	    "Failed(%d) to gain access to registers at %p: %s.\n",
	    ret, str_error(ret), io);
	instance->registers = io;
	usb_log_debug("Device registers at %p(%u) accessible.\n",
	    io, reg_size);

	ret = hc_init_mem_structures(instance);
	CHECK_RET_DEST_FUN_RETURN(ret,
	    "Failed to initialize UHCI memory structures.\n");

	hc_init_hw(instance);
	if (!interrupts) {
		instance->cleaner =
		    fibril_create(hc_interrupt_emulator, instance);
		fibril_add_ready(instance->cleaner);
	} else {
		/* TODO: enable interrupts here */
	}

	instance->debug_checker =
	    fibril_create(hc_debug_checker, instance);
//	fibril_add_ready(instance->debug_checker);

	return EOK;
#undef CHECK_RET_DEST_FUN_RETURN
}
/*----------------------------------------------------------------------------*/
/** Initialize UHCI hc hw resources.
 *
 * @param[in] instance UHCI structure to use.
 * For magic values see UHCI Design Guide
 */
void hc_init_hw(hc_t *instance)
{
	assert(instance);
	regs_t *registers = instance->registers;

	/* Reset everything, who knows what touched it before us */
	pio_write_16(&registers->usbcmd, UHCI_CMD_GLOBAL_RESET);
	async_usleep(10000); /* 10ms according to USB spec */
	pio_write_16(&registers->usbcmd, 0);

	/* Reset hc, all states and counters */
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
		    UHCI_INTR_CRC | UHCI_INTR_COMPLETE | UHCI_INTR_SHORT_PACKET);
	}

	uint16_t status = pio_read_16(&registers->usbcmd);
	if (status != 0)
		usb_log_warning("Previous command value: %x.\n", status);

	/* Start the hc with large(64B) packet FSBR */
	pio_write_16(&registers->usbcmd,
	    UHCI_CMD_RUN_STOP | UHCI_CMD_MAX_PACKET | UHCI_CMD_CONFIGURE);
}
/*----------------------------------------------------------------------------*/
/** Initialize UHCI hc memory structures.
 *
 * @param[in] instance UHCI structure to use.
 * @return Error code
 * @note Should be called only once on any structure.
 *
 * Structures:
 *  - interrupt code (I/O addressses are customized per instance)
 *  - transfer lists (queue heads need to be accessible by the hw)
 *  - frame list page (needs to be one UHCI hw accessible 4K page)
 */
int hc_init_mem_structures(hc_t *instance)
{
	assert(instance);
#define CHECK_RET_DEST_CMDS_RETURN(ret, message...) \
	if (ret != EOK) { \
		usb_log_error(message); \
		if (instance->interrupt_code.cmds != NULL) \
			free(instance->interrupt_code.cmds); \
		return ret; \
	} else (void) 0

	/* Init interrupt code */
	instance->interrupt_code.cmds = malloc(sizeof(uhci_cmds));
	int ret = (instance->interrupt_code.cmds == NULL) ? ENOMEM : EOK;
	CHECK_RET_DEST_CMDS_RETURN(ret,
	    "Failed to allocate interrupt cmds space.\n");

	{
		irq_cmd_t *interrupt_commands = instance->interrupt_code.cmds;
		memcpy(interrupt_commands, uhci_cmds, sizeof(uhci_cmds));
		interrupt_commands[0].addr =
		    (void*)&instance->registers->usbsts;
		interrupt_commands[1].addr =
		    (void*)&instance->registers->usbsts;
		instance->interrupt_code.cmdcount =
		    sizeof(uhci_cmds) / sizeof(irq_cmd_t);
	}

	/* Init transfer lists */
	ret = hc_init_transfer_lists(instance);
	CHECK_RET_DEST_CMDS_RETURN(ret, "Failed to init transfer lists.\n");
	usb_log_debug("Initialized transfer lists.\n");

	/* Init USB frame list page*/
	instance->frame_list = get_page();
	ret = instance ? EOK : ENOMEM;
	CHECK_RET_DEST_CMDS_RETURN(ret, "Failed to get frame list page.\n");
	usb_log_debug("Initialized frame list at %p.\n", instance->frame_list);

	/* Set all frames to point to the first queue head */
	const uint32_t queue =
	  instance->transfers_interrupt.queue_head_pa
	  | LINK_POINTER_QUEUE_HEAD_FLAG;

	unsigned i = 0;
	for(; i < UHCI_FRAME_LIST_COUNT; ++i) {
		instance->frame_list[i] = queue;
	}

	/* Init device keeper*/
	usb_device_keeper_init(&instance->manager);
	usb_log_debug("Initialized device manager.\n");

	ret = bandwidth_init(&instance->bandwidth, BANDWIDTH_AVAILABLE_USB11,
	    bandwidth_count_usb11);
	assert(ret == EOK);

	return EOK;
#undef CHECK_RET_DEST_CMDS_RETURN
}
/*----------------------------------------------------------------------------*/
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
#define CHECK_RET_CLEAR_RETURN(ret, message...) \
	if (ret != EOK) { \
		usb_log_error(message); \
		transfer_list_fini(&instance->transfers_bulk_full); \
		transfer_list_fini(&instance->transfers_control_full); \
		transfer_list_fini(&instance->transfers_control_slow); \
		transfer_list_fini(&instance->transfers_interrupt); \
		return ret; \
	} else (void) 0

	/* initialize TODO: check errors */
	int ret;
	ret = transfer_list_init(&instance->transfers_bulk_full, "BULK_FULL");
	CHECK_RET_CLEAR_RETURN(ret, "Failed to init BULK list.");

	ret = transfer_list_init(
	    &instance->transfers_control_full, "CONTROL_FULL");
	CHECK_RET_CLEAR_RETURN(ret, "Failed to init CONTROL FULL list.");

	ret = transfer_list_init(
	    &instance->transfers_control_slow, "CONTROL_SLOW");
	CHECK_RET_CLEAR_RETURN(ret, "Failed to init CONTROL SLOW list.");

	ret = transfer_list_init(&instance->transfers_interrupt, "INTERRUPT");
	CHECK_RET_CLEAR_RETURN(ret, "Failed to init INTERRUPT list.");

	transfer_list_set_next(&instance->transfers_control_full,
		&instance->transfers_bulk_full);
	transfer_list_set_next(&instance->transfers_control_slow,
		&instance->transfers_control_full);
	transfer_list_set_next(&instance->transfers_interrupt,
		&instance->transfers_control_slow);

	/*FSBR*/
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

	return EOK;
#undef CHECK_RET_CLEAR_RETURN
}
/*----------------------------------------------------------------------------*/
/** Schedule batch for execution.
 *
 * @param[in] instance UHCI structure to use.
 * @param[in] batch Transfer batch to schedule.
 * @return Error code
 *
 * Checks for bandwidth availability and appends the batch to the proper queue.
 */
int hc_schedule(hc_t *instance, usb_transfer_batch_t *batch)
{
	assert(instance);
	assert(batch);
	const int low_speed = (batch->speed == USB_SPEED_LOW);
	if (!usb_is_allowed(
	    low_speed, batch->transfer_type, batch->max_packet_size)) {
		usb_log_error("Invalid USB transfer specified %s %d %zu.\n",
		    usb_str_speed(batch->speed), batch->transfer_type,
		    batch->max_packet_size);
		return ENOTSUP;
	}
	/* Check available bandwidth */
	if (batch->transfer_type == USB_TRANSFER_INTERRUPT ||
	    batch->transfer_type == USB_TRANSFER_ISOCHRONOUS) {
		int ret =
		    bandwidth_use(&instance->bandwidth, batch->target.address,
		    batch->target.endpoint, batch->direction);
		if (ret != EOK) {
			usb_log_warning("Failed(%d) to use reserved bw: %s.\n",
			    ret, str_error(ret));
		}
	}

	transfer_list_t *list =
	    instance->transfers[batch->speed][batch->transfer_type];
	assert(list);
	if (batch->transfer_type == USB_TRANSFER_CONTROL) {
		usb_device_keeper_use_control(
		    &instance->manager, batch->target);
	}
	transfer_list_add_batch(list, batch);

	return EOK;
}
/*----------------------------------------------------------------------------*/
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
//	status |= 1; //Uncomment to work around qemu hang
	/* TODO: Resume interrupts are not supported */
	/* Lower 2 bits are transaction error and transaction complete */
	if (status & 0x3) {
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
			link_t *item = done.next;
			list_remove(item);
			usb_transfer_batch_t *batch =
			    list_get_instance(item, usb_transfer_batch_t, link);
			switch (batch->transfer_type)
			{
			case USB_TRANSFER_CONTROL:
				usb_device_keeper_release_control(
				    &instance->manager, batch->target);
				break;
			case USB_TRANSFER_INTERRUPT:
			case USB_TRANSFER_ISOCHRONOUS: {
				int ret = bandwidth_free(&instance->bandwidth,
				    batch->target.address,
				    batch->target.endpoint,
				    batch->direction);
				if (ret != EOK)
					usb_log_warning("Failed(%d) to free "
					    "reserved bw: %s.\n", ret,
					    str_error(ret));
				}
			default:
				break;
			}
			batch->next_step(batch);
		}
	}
	/* bits 4 and 5 indicate hc error */
	if (status & 0x18) {
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
/*----------------------------------------------------------------------------*/
/** Polling function, emulates interrupts.
 *
 * @param[in] arg UHCI hc structure to use.
 * @return EOK (should never return)
 */
int hc_interrupt_emulator(void* arg)
{
	usb_log_debug("Started interrupt emulator.\n");
	hc_t *instance = (hc_t*)arg;
	assert(instance);

	while (1) {
		/* read and ack interrupts */
		uint16_t status = pio_read_16(&instance->registers->usbsts);
		pio_write_16(&instance->registers->usbsts, 0x1f);
		if (status != 0)
			usb_log_debug2("UHCI status: %x.\n", status);
		hc_interrupt(instance, status);
		async_usleep(UHCI_CLEANER_TIMEOUT);
	}
	return EOK;
}
/*---------------------------------------------------------------------------*/
/** Debug function, checks consistency of memory structures.
 *
 * @param[in] arg UHCI structure to use.
 * @return EOK (should never return)
 */
int hc_debug_checker(void *arg)
{
	hc_t *instance = (hc_t*)arg;
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

		uintptr_t frame_list =
		    pio_read_32(&instance->registers->flbaseadd) & ~0xfff;
		if (frame_list != addr_to_phys(instance->frame_list)) {
			usb_log_debug("Framelist address: %p vs. %p.\n",
			    frame_list, addr_to_phys(instance->frame_list));
		}

		int frnum = pio_read_16(&instance->registers->frnum) & 0x3ff;

		uintptr_t expected_pa = instance->frame_list[frnum]
		    & LINK_POINTER_ADDRESS_MASK;
		uintptr_t real_pa = addr_to_phys(QH(interrupt));
		if (expected_pa != real_pa) {
			usb_log_debug("Interrupt QH: %p(frame: %d) vs. %p.\n",
			    expected_pa, frnum, real_pa);
		}

		expected_pa = QH(interrupt)->next & LINK_POINTER_ADDRESS_MASK;
		real_pa = addr_to_phys(QH(control_slow));
		if (expected_pa != real_pa) {
			usb_log_debug("Control Slow QH: %p vs. %p.\n",
			    expected_pa, real_pa);
		}

		expected_pa = QH(control_slow)->next & LINK_POINTER_ADDRESS_MASK;
		real_pa = addr_to_phys(QH(control_full));
		if (expected_pa != real_pa) {
			usb_log_debug("Control Full QH: %p vs. %p.\n",
			    expected_pa, real_pa);
		}

		expected_pa = QH(control_full)->next & LINK_POINTER_ADDRESS_MASK;
		real_pa = addr_to_phys(QH(bulk_full));
		if (expected_pa != real_pa ) {
			usb_log_debug("Bulk QH: %p vs. %p.\n",
			    expected_pa, real_pa);
		}
		async_usleep(UHCI_DEBUGER_TIMEOUT);
	}
	return EOK;
#undef QH
}
/*----------------------------------------------------------------------------*/
/** Check transfers for USB validity
 *
 * @param[in] low_speed Transfer speed.
 * @param[in] transfer Transer type
 * @param[in] size Size of data packets
 * @return True if transaction is allowed by USB specs, false otherwise
 */
bool usb_is_allowed(
    bool low_speed, usb_transfer_type_t transfer, size_t size)
{
	/* see USB specification chapter 5.5-5.8 for magic numbers used here */
	switch(transfer)
	{
	case USB_TRANSFER_ISOCHRONOUS:
		return (!low_speed && size < 1024);
	case USB_TRANSFER_INTERRUPT:
		return size <= (low_speed ? 8 : 64);
	case USB_TRANSFER_CONTROL: /* device specifies its own max size */
		return (size <= (low_speed ? 8 : 64));
	case USB_TRANSFER_BULK: /* device specifies its own max size */
		return (!low_speed && size <= 64);
	}
	return false;
}
/**
 * @}
 */
