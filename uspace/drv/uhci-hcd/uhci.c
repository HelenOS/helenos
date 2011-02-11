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
static int uhci_clean_finished(void *arg);
static int uhci_debug_checker(void *arg);
static bool allowed_usb_packet(
	bool low_speed, usb_transfer_type_t, size_t size);

int uhci_init(uhci_t *instance, void *regs, size_t reg_size)
{
#define CHECK_RET_RETURN(message...) \
	if (ret != EOK) { \
		usb_log_error(message); \
		return ret; \
	} else (void) 0

	/* init address keeper(libusb) */
	usb_address_keeping_init(&instance->address_manager, USB11_ADDRESS_MAX);
	usb_log_debug("Initialized address manager.\n");

	/* allow access to hc control registers */
	regs_t *io;
	assert(reg_size >= sizeof(regs_t));
	int ret = pio_enable(regs, reg_size, (void**)&io);
	CHECK_RET_RETURN("Failed to gain access to registers at %p.\n", io);
	instance->registers = io;
	usb_log_debug("Device registers accessible.\n");

	/* init transfer lists */
	ret = uhci_init_transfer_lists(instance);
	CHECK_RET_RETURN("Failed to initialize transfer lists.\n");
	usb_log_debug("Transfer lists initialized.\n");


	usb_log_debug("Initializing frame list.\n");
	instance->frame_list = get_page();
	ret = instance ? EOK : ENOMEM;
	CHECK_RET_RETURN("Failed to get frame list page.\n");

	/* initialize all frames to point to the first queue head */
	const uint32_t queue =
	  instance->transfers_interrupt.queue_head_pa
	  | LINK_POINTER_QUEUE_HEAD_FLAG;
	unsigned i = 0;
	for(; i < UHCI_FRAME_LIST_COUNT; ++i) {
		instance->frame_list[i] = queue;
	}

	const uintptr_t pa = (uintptr_t)addr_to_phys(instance->frame_list);
	pio_write_32(&instance->registers->flbaseadd, (uint32_t)pa);

	list_initialize(&instance->tracker_list);
	fibril_mutex_initialize(&instance->tracker_list_mutex);

	instance->cleaner = fibril_create(uhci_clean_finished, instance);
	fibril_add_ready(instance->cleaner);

	instance->debug_checker = fibril_create(uhci_debug_checker, instance);
	fibril_add_ready(instance->debug_checker);

	/* Start the hc with large(64B) packet FSBR */
	pio_write_16(&instance->registers->usbcmd,
	    UHCI_CMD_RUN_STOP | UHCI_CMD_MAX_PACKET);
	usb_log_debug("Started UHCI HC.\n");

	uint16_t cmd = pio_read_16(&instance->registers->usbcmd);
	cmd |= UHCI_CMD_DEBUG;
	pio_write_16(&instance->registers->usbcmd, cmd);

	return EOK;
}
/*----------------------------------------------------------------------------*/
int uhci_init_transfer_lists(uhci_t *instance)
{
	assert(instance);

	/* initialize */
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
	instance->transfers[0][USB_TRANSFER_CONTROL] =
	  &instance->transfers_control_full;

	return EOK;
}
/*----------------------------------------------------------------------------*/
int uhci_schedule(uhci_t *instance, tracker_t *tracker)
{
	assert(instance);
	assert(tracker);
	const int low_speed = (tracker->speed == LOW_SPEED);
	if (!allowed_usb_packet(
	    low_speed, tracker->transfer_type, tracker->packet_size)) {
		usb_log_warning("Invalid USB packet specified %s SPEED %d %zu.\n",
			  low_speed ? "LOW" : "FULL" , tracker->transfer_type,
		    tracker->packet_size);
		return ENOTSUP;
	}
	/* TODO: check available bandwith here */

	usb_log_debug2("Scheduler(%d) acquiring tracker list mutex.\n",
	    fibril_get_id());
	fibril_mutex_lock(&instance->tracker_list_mutex);
	usb_log_debug2("Scheduler(%d) acquired tracker list mutex.\n",
	    fibril_get_id());

	transfer_list_t *list =
	    instance->transfers[low_speed][tracker->transfer_type];
	assert(list);
	transfer_list_add_tracker(list, tracker);
	list_append(&tracker->link, &instance->tracker_list);

	tracker->scheduled_list = list;

	usb_log_debug2("Scheduler(%d) releasing tracker list mutex.\n",
	    fibril_get_id());
	fibril_mutex_unlock(&instance->tracker_list_mutex);
	usb_log_debug2("Scheduler(%d) released tracker list mutex.\n",
	    fibril_get_id());

	return EOK;
}
/*----------------------------------------------------------------------------*/
int uhci_clean_finished(void* arg)
{
	usb_log_debug("Started cleaning fibril.\n");
	uhci_t *instance = (uhci_t*)arg;
	assert(instance);

	while(1) {
		LIST_INITIALIZE(done_trackers);
		/* tracker iteration */

		usb_log_debug2("Cleaner(%d) acquiring tracker list mutex.\n",
		    fibril_get_id());
		fibril_mutex_lock(&instance->tracker_list_mutex);
		usb_log_debug2("Cleaner(%d) acquired tracker list mutex.\n",
		    fibril_get_id());

		link_t *current = instance->tracker_list.next;
		while (current != &instance->tracker_list)
		{

			link_t *next = current->next;
			tracker_t *tracker = list_get_instance(current, tracker_t, link);

			assert(current == &tracker->link);
			assert(tracker);
			assert(tracker->next_step);
			assert(tracker->td);

			if (!transfer_descriptor_is_active(tracker->td)) {
				usb_log_info("Found inactive tracker with status: %x:%x.\n",
				    tracker->td->status, tracker->td->device);
				list_remove(current);
				list_append(current, &done_trackers);
			}
			current = next;
		}

		usb_log_debug2("Cleaner(%d) releasing tracker list mutex.\n",
		    fibril_get_id());
		fibril_mutex_unlock(&instance->tracker_list_mutex);
		usb_log_debug2("Cleaner(%d) released tracker list mutex.\n",
		    fibril_get_id());

		while (!list_empty(&done_trackers)) {
			tracker_t *tracker = list_get_instance(
			  done_trackers.next, tracker_t, link);
			list_remove(&tracker->link);
			tracker->next_step(tracker);
		}
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
		uint16_t cmd = pio_read_16(&instance->registers->usbcmd);
		uint16_t sts = pio_read_16(&instance->registers->usbsts);
		usb_log_debug("Command register: %X Status register: %X\n", cmd, sts);

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
