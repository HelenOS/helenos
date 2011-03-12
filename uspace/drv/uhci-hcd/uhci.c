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
/** @addtogroup usb
 * @{
 */
/** @file
 * @brief UHCI driver
 */
#include <errno.h>
#include <str_error.h>
#include <adt/list.h>
#include <libarch/ddi.h>

#include <usb/debug.h>
#include <usb/usb.h>
#include <usb/ddfiface.h>
#include <usb_iface.h>

#include "uhci.h"
#include "iface.h"

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

/** Gets USB address of the calling device.
 *
 * @param[in] fun UHCI hc function.
 * @param[in] handle Handle of the device seeking address.
 * @param[out] address Place to store found address.
 * @return Error code.
 */
static int usb_iface_get_address(
    ddf_fun_t *fun, devman_handle_t handle, usb_address_t *address)
{
	assert(fun);
	uhci_t *hc = fun_to_uhci(fun);
	assert(hc);

	usb_address_t addr = device_keeper_find(&hc->device_manager,
	    handle);
	if (addr < 0) {
		return addr;
	}

	if (address != NULL) {
		*address = addr;
	}

	return EOK;
}
/*----------------------------------------------------------------------------*/
static usb_iface_t hc_usb_iface = {
	.get_hc_handle = usb_iface_get_hc_handle_hc_impl,
	.get_address = usb_iface_get_address
};
/*----------------------------------------------------------------------------*/
static ddf_dev_ops_t uhci_ops = {
	.interfaces[USB_DEV_IFACE] = &hc_usb_iface,
	.interfaces[USBHC_DEV_IFACE] = &uhci_iface,
};
/*----------------------------------------------------------------------------*/
static int uhci_init_transfer_lists(uhci_t *instance);
static int uhci_init_mem_structures(uhci_t *instance);
static void uhci_init_hw(uhci_t *instance);

static int uhci_interrupt_emulator(void *arg);
static int uhci_debug_checker(void *arg);

static bool allowed_usb_packet(
    bool low_speed, usb_transfer_type_t transfer, size_t size);
/*----------------------------------------------------------------------------*/
/** Initializes UHCI hcd driver structure
 *
 * @param[in] instance Memory place to initialize.
 * @param[in] dev DDF device.
 * @param[in] regs Address of I/O control registers.
 * @param[in] size Size of I/O control registers.
 * @return Error code.
 * @note Should be called only once on any structure.
 */
int uhci_init(uhci_t *instance, ddf_dev_t *dev, void *regs, size_t reg_size)
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

	/* Create UHCI function. */
	instance->ddf_instance = ddf_fun_create(dev, fun_exposed, "uhci");
	ret = (instance->ddf_instance == NULL) ? ENOMEM : EOK;
	CHECK_RET_DEST_FUN_RETURN(ret,
	    "Failed to create UHCI device function.\n");

	instance->ddf_instance->ops = &uhci_ops;
	instance->ddf_instance->driver_data = instance;

	ret = ddf_fun_bind(instance->ddf_instance);
	CHECK_RET_DEST_FUN_RETURN(ret,
	    "Failed(%d) to bind UHCI device function: %s.\n",
	    ret, str_error(ret));

	/* allow access to hc control registers */
	regs_t *io;
	ret = pio_enable(regs, reg_size, (void**)&io);
	CHECK_RET_DEST_FUN_RETURN(ret,
	    "Failed(%d) to gain access to registers at %p: %s.\n",
	    ret, str_error(ret), io);
	instance->registers = io;
	usb_log_debug("Device registers at %p(%u) accessible.\n",
	    io, reg_size);

	ret = uhci_init_mem_structures(instance);
	CHECK_RET_DEST_FUN_RETURN(ret,
	    "Failed to initialize UHCI memory structures.\n");

	uhci_init_hw(instance);
	instance->cleaner =
	    fibril_create(uhci_interrupt_emulator, instance);
	fibril_add_ready(instance->cleaner);

	instance->debug_checker = fibril_create(uhci_debug_checker, instance);
	fibril_add_ready(instance->debug_checker);

	usb_log_info("Started UHCI driver.\n");
	return EOK;
#undef CHECK_RET_DEST_FUN_RETURN
}
/*----------------------------------------------------------------------------*/
/** Initializes UHCI hcd hw resources.
 *
 * @param[in] instance UHCI structure to use.
 */
void uhci_init_hw(uhci_t *instance)
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

	/* Set framelist pointer */
	const uint32_t pa = addr_to_phys(instance->frame_list);
	pio_write_32(&registers->flbaseadd, pa);

	/* Enable all interrupts, but resume interrupt */
//	pio_write_16(&instance->registers->usbintr,
//	    UHCI_INTR_CRC | UHCI_INTR_COMPLETE | UHCI_INTR_SHORT_PACKET);

	uint16_t status = pio_read_16(&registers->usbcmd);
	if (status != 0)
		usb_log_warning("Previous command value: %x.\n", status);

	/* Start the hc with large(64B) packet FSBR */
	pio_write_16(&registers->usbcmd,
	    UHCI_CMD_RUN_STOP | UHCI_CMD_MAX_PACKET | UHCI_CMD_CONFIGURE);
}
/*----------------------------------------------------------------------------*/
/** Initializes UHCI hcd memory structures.
 *
 * @param[in] instance UHCI structure to use.
 * @return Error code
 * @note Should be called only once on any structure.
 */
int uhci_init_mem_structures(uhci_t *instance)
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
	ret = uhci_init_transfer_lists(instance);
	CHECK_RET_DEST_CMDS_RETURN(ret, "Failed to init transfer lists.\n");
	usb_log_debug("Initialized transfer lists.\n");

	/* Init USB frame list page*/
	instance->frame_list = get_page();
	ret = instance ? EOK : ENOMEM;
	CHECK_RET_DEST_CMDS_RETURN(ret, "Failed to get frame list page.\n");
	usb_log_debug("Initialized frame list.\n");

	/* Set all frames to point to the first queue head */
	const uint32_t queue =
	  instance->transfers_interrupt.queue_head_pa
	  | LINK_POINTER_QUEUE_HEAD_FLAG;

	unsigned i = 0;
	for(; i < UHCI_FRAME_LIST_COUNT; ++i) {
		instance->frame_list[i] = queue;
	}

	/* Init device keeper*/
	device_keeper_init(&instance->device_manager);
	usb_log_debug("Initialized device manager.\n");

	return EOK;
#undef CHECK_RET_DEST_CMDS_RETURN
}
/*----------------------------------------------------------------------------*/
/** Initializes UHCI hcd transfer lists.
 *
 * @param[in] instance UHCI structure to use.
 * @return Error code
 * @note Should be called only once on any structure.
 */
int uhci_init_transfer_lists(uhci_t *instance)
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
/** Schedules batch for execution.
 *
 * @param[in] instance UHCI structure to use.
 * @param[in] batch Transfer batch to schedule.
 * @return Error code
 */
int uhci_schedule(uhci_t *instance, batch_t *batch)
{
	assert(instance);
	assert(batch);
	const int low_speed = (batch->speed == USB_SPEED_LOW);
	if (!allowed_usb_packet(
	    low_speed, batch->transfer_type, batch->max_packet_size)) {
		usb_log_warning(
		    "Invalid USB packet specified %s SPEED %d %zu.\n",
		    low_speed ? "LOW" : "FULL" , batch->transfer_type,
		    batch->max_packet_size);
		return ENOTSUP;
	}
	/* TODO: check available bandwith here */

	transfer_list_t *list =
	    instance->transfers[batch->speed][batch->transfer_type];
	assert(list);
	transfer_list_add_batch(list, batch);

	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Takes action based on the interrupt cause.
 *
 * @param[in] instance UHCI structure to use.
 * @param[in] status Value of the stsatus regiser at the time of interrupt.
 */
void uhci_interrupt(uhci_t *instance, uint16_t status)
{
	assert(instance);
	/* TODO: Check interrupt cause here */
	/* Lower 2 bits are transaction error and transaction complete */
	if (status & 0x3) {
		transfer_list_remove_finished(&instance->transfers_interrupt);
		transfer_list_remove_finished(&instance->transfers_control_slow);
		transfer_list_remove_finished(&instance->transfers_control_full);
		transfer_list_remove_finished(&instance->transfers_bulk_full);
	}
}
/*----------------------------------------------------------------------------*/
/** Polling function, emulates interrupts.
 *
 * @param[in] arg UHCI structure to use.
 * @return EOK
 */
int uhci_interrupt_emulator(void* arg)
{
	usb_log_debug("Started interrupt emulator.\n");
	uhci_t *instance = (uhci_t*)arg;
	assert(instance);

	while (1) {
		/* read and ack interrupts */
		uint16_t status = pio_read_16(&instance->registers->usbsts);
		pio_write_16(&instance->registers->usbsts, 0x1f);
		if (status != 0)
			usb_log_debug2("UHCI status: %x.\n", status);
		uhci_interrupt(instance, status);
		async_usleep(UHCI_CLEANER_TIMEOUT);
	}
	return EOK;
}
/*---------------------------------------------------------------------------*/
/** Debug function, checks consistency of memory structures.
 *
 * @param[in] arg UHCI structure to use.
 * @return EOK
 */
int uhci_debug_checker(void *arg)
{
	uhci_t *instance = (uhci_t*)arg;
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

		uintptr_t expected_pa = instance->frame_list[frnum] & (~0xf);
		uintptr_t real_pa = addr_to_phys(QH(interrupt));
		if (expected_pa != real_pa) {
			usb_log_debug("Interrupt QH: %p(frame: %d) vs. %p.\n",
			    expected_pa, frnum, real_pa);
		}

		expected_pa = QH(interrupt)->next_queue & (~0xf);
		real_pa = addr_to_phys(QH(control_slow));
		if (expected_pa != real_pa) {
			usb_log_debug("Control Slow QH: %p vs. %p.\n",
			    expected_pa, real_pa);
		}

		expected_pa = QH(control_slow)->next_queue & (~0xf);
		real_pa = addr_to_phys(QH(control_full));
		if (expected_pa != real_pa) {
			usb_log_debug("Control Full QH: %p vs. %p.\n",
			    expected_pa, real_pa);
		}

		expected_pa = QH(control_full)->next_queue & (~0xf);
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
/** Checks transfer packets, for USB validity
 *
 * @param[in] low_speed Transfer speed.
 * @param[in] transfer Transer type
 * @param[in] size Maximum size of used packets
 * @return EOK
 */
bool allowed_usb_packet(
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
