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
#include <adt/list.h>

#include <usb/debug.h>
#include <usb/usb.h>

#include "uhci.h"

static int uhci_init_transfer_lists(uhci_t *instance);
static int uhci_init_mem_structures(uhci_t *instance);
static void uhci_init_hw(uhci_t *instance);

static int uhci_interrupt_emulator(void *arg);
static int uhci_debug_checker(void *arg);

static bool allowed_usb_packet(
	bool low_speed, usb_transfer_type_t, size_t size);

#define CHECK_RET_RETURN(ret, message...) \
	if (ret != EOK) { \
		usb_log_error(message); \
		return ret; \
	} else (void) 0

int uhci_init(uhci_t *instance, void *regs, size_t reg_size)
{
	int ret = uhci_init_mem_structures(instance);
	CHECK_RET_RETURN(ret, "Failed to initialize memory structures.\n");

	/* allow access to hc control registers */
	regs_t *io;
	assert(reg_size >= sizeof(regs_t));
	ret = pio_enable(regs, reg_size, (void**)&io);
	CHECK_RET_RETURN(ret, "Failed to gain access to registers at %p.\n", io);
	instance->registers = io;
	usb_log_debug("Device registers accessible.\n");

	instance->cleaner = fibril_create(uhci_interrupt_emulator, instance);
	fibril_add_ready(instance->cleaner);

	instance->debug_checker = fibril_create(uhci_debug_checker, instance);
	fibril_add_ready(instance->debug_checker);

	uhci_init_hw(instance);

	return EOK;
}
/*----------------------------------------------------------------------------*/
void uhci_init_hw(uhci_t *instance)
{
	const uint32_t pa = addr_to_phys(instance->frame_list);
	pio_write_32(&instance->registers->flbaseadd, pa);

	/* enable all interrupts */
	pio_write_16(&instance->registers->usbintr,
		  UHCI_INTR_CRC | UHCI_INTR_COMPLETE | UHCI_INTR_SHORT_PACKET);

	/* Start the hc with large(64B) packet FSBR */
	pio_write_16(&instance->registers->usbcmd,
	    UHCI_CMD_RUN_STOP | UHCI_CMD_MAX_PACKET);
	usb_log_debug("Started UHCI HC.\n");
}
/*----------------------------------------------------------------------------*/
int uhci_init_mem_structures(uhci_t *instance)
{
	assert(instance);
	/* init transfer lists */
	int ret = uhci_init_transfer_lists(instance);
	CHECK_RET_RETURN(ret, "Failed to initialize transfer lists.\n");
	usb_log_debug("Initialized transfer lists.\n");

	/* frame list initialization */
	instance->frame_list = get_page();
	ret = instance ? EOK : ENOMEM;
	CHECK_RET_RETURN(ret, "Failed to get frame list page.\n");
	usb_log_debug("Initialized frame list.\n");

	/* initialize all frames to point to the first queue head */
	const uint32_t queue =
	  instance->transfers_interrupt.queue_head_pa
	  | LINK_POINTER_QUEUE_HEAD_FLAG;
	unsigned i = 0;
	for(; i < UHCI_FRAME_LIST_COUNT; ++i) {
		instance->frame_list[i] = queue;
	}

	/* init address keeper(libusb) */
	usb_address_keeping_init(&instance->address_manager, USB11_ADDRESS_MAX);
	usb_log_debug("Initialized address manager.\n");

	return EOK;
}
/*----------------------------------------------------------------------------*/
int uhci_init_transfer_lists(uhci_t *instance)
{
	assert(instance);

	/* initialize TODO: check errors */
	int ret;
	ret = transfer_list_init(&instance->transfers_bulk_full, "BULK_FULL");
	assert(ret == EOK);
	ret = transfer_list_init(&instance->transfers_control_full, "CONTROL_FULL");
	assert(ret == EOK);
	ret = transfer_list_init(&instance->transfers_control_slow, "CONTROL_SLOW");
	assert(ret == EOK);
	ret = transfer_list_init(&instance->transfers_interrupt, "INTERRUPT");
	assert(ret == EOK);

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

	instance->transfers[0][USB_TRANSFER_INTERRUPT] =
	  &instance->transfers_interrupt;
	instance->transfers[1][USB_TRANSFER_INTERRUPT] =
	  &instance->transfers_interrupt;
	instance->transfers[0][USB_TRANSFER_CONTROL] =
	  &instance->transfers_control_full;
	instance->transfers[1][USB_TRANSFER_CONTROL] =
	  &instance->transfers_control_slow;
	instance->transfers[0][USB_TRANSFER_BULK] =
	  &instance->transfers_bulk_full;

	return EOK;
}
/*----------------------------------------------------------------------------*/
int uhci_schedule(uhci_t *instance, batch_t *batch)
{
	assert(instance);
	assert(batch);
	const int low_speed = (batch->speed == LOW_SPEED);
	if (!allowed_usb_packet(
	    low_speed, batch->transfer_type, batch->max_packet_size)) {
		usb_log_warning("Invalid USB packet specified %s SPEED %d %zu.\n",
			  low_speed ? "LOW" : "FULL" , batch->transfer_type,
		    batch->max_packet_size);
		return ENOTSUP;
	}
	/* TODO: check available bandwith here */

	transfer_list_t *list =
	    instance->transfers[low_speed][batch->transfer_type];
	assert(list);
	transfer_list_add_batch(list, batch);

	return EOK;
}
/*----------------------------------------------------------------------------*/
void uhci_interrupt(uhci_t *instance)
{
	assert(instance);
	const uint16_t sts = pio_read_16(&instance->registers->usbsts);
	if ((sts & (UHCI_STATUS_INTERRUPT | UHCI_STATUS_ERROR_INTERRUPT)) == 0)
		return;
	usb_log_debug("UHCI interrupt: %X.\n", sts);
	transfer_list_check(&instance->transfers_interrupt);
	transfer_list_check(&instance->transfers_control_slow);
	transfer_list_check(&instance->transfers_control_full);
	transfer_list_check(&instance->transfers_bulk_full);
	pio_write_16(&instance->registers->usbsts, 0xf);
}
/*----------------------------------------------------------------------------*/
int uhci_interrupt_emulator(void* arg)
{
	return EOK;
	usb_log_debug("Started interrupt emulator.\n");
	uhci_t *instance = (uhci_t*)arg;
	assert(instance);

	while(1) {
		uhci_interrupt(instance);
		async_usleep(UHCI_CLEANER_TIMEOUT);
	}
	return EOK;
}
/*---------------------------------------------------------------------------*/
int uhci_debug_checker(void *arg)
{
	uhci_t *instance = (uhci_t*)arg;
	assert(instance);
	while (1) {
		const uint16_t cmd = pio_read_16(&instance->registers->usbcmd);
		const uint16_t sts = pio_read_16(&instance->registers->usbsts);
		const uint16_t intr = pio_read_16(&instance->registers->usbintr);
		usb_log_debug("Command: %X Status: %X Interrupts: %x\n",
		    cmd, sts, intr);

		uintptr_t frame_list = pio_read_32(&instance->registers->flbaseadd);
		if (frame_list != (uintptr_t)addr_to_phys(instance->frame_list)) {
			usb_log_debug("Framelist address: %p vs. %p.\n",
				frame_list, addr_to_phys(instance->frame_list));
		}
		int frnum = pio_read_16(&instance->registers->frnum) & 0x3ff;
		usb_log_debug2("Framelist item: %d \n", frnum );

		queue_head_t* qh = instance->transfers_interrupt.queue_head;

		if ((instance->frame_list[frnum] & (~0xf)) != (uintptr_t)addr_to_phys(qh)) {
			usb_log_debug("Interrupt QH: %p vs. %p.\n",
				instance->frame_list[frnum] & (~0xf), addr_to_phys(qh));
		}

		if ((qh->next_queue & (~0xf))
		  != (uintptr_t)addr_to_phys(instance->transfers_control_slow.queue_head)) {
			usb_log_debug("Control Slow QH: %p vs. %p.\n", qh->next_queue & (~0xf),
				addr_to_phys(instance->transfers_control_slow.queue_head));
		}
		qh = instance->transfers_control_slow.queue_head;

		if ((qh->next_queue & (~0xf))
		  != (uintptr_t)addr_to_phys(instance->transfers_control_full.queue_head)) {
			usb_log_debug("Control Full QH: %p vs. %p.\n", qh->next_queue & (~0xf),
				addr_to_phys(instance->transfers_control_full.queue_head));\
		}
		qh = instance->transfers_control_full.queue_head;

		if ((qh->next_queue & (~0xf))
		  != (uintptr_t)addr_to_phys(instance->transfers_bulk_full.queue_head)) {
			usb_log_debug("Bulk QH: %p vs. %p.\n", qh->next_queue & (~0xf),
				addr_to_phys(instance->transfers_bulk_full.queue_head));
		}
/*
	uint16_t cmd = pio_read_16(&instance->registers->usbcmd);
	cmd |= UHCI_CMD_RUN_STOP;
	pio_write_16(&instance->registers->usbcmd, cmd);
*/
		async_usleep(UHCI_DEBUGER_TIMEOUT);
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
bool allowed_usb_packet(
	bool low_speed, usb_transfer_type_t transfer, size_t size)
{
	/* see USB specification chapter 5.5-5.8 for magic numbers used here */
	switch(transfer) {
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
