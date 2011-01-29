#include <errno.h>
#include <usb/debug.h>
#include <usb/usb.h>

#include "utils/malloc32.h"

#include "debug.h"
#include "name.h"
#include "uhci.h"

static int uhci_init_transfer_lists(transfer_list_t list[]);
static int uhci_clean_finished(void *arg);
static int uhci_debug_checker(void *arg);

int uhci_init(device_t *device, void *regs)
{
	assert(device);
	uhci_print_info("Initializing device at address %p.\n", device);

#define CHECK_RET_FREE_INSTANCE(message...) \
	if (ret != EOK) { \
		uhci_print_error(message); \
		if (instance) { \
			free(instance); \
		} \
		return ret; \
	} else (void) 0

	/* create instance */
	uhci_t *instance = malloc(sizeof(uhci_t));
	int ret = instance ? EOK : ENOMEM;
	CHECK_RET_FREE_INSTANCE("Failed to allocate uhci driver instance.\n");

	bzero(instance, sizeof(uhci_t));

	/* init address keeper(libusb) */
	usb_address_keeping_init(&instance->address_manager, USB11_ADDRESS_MAX);
	uhci_print_verbose("Initialized address manager.\n");

	/* allow access to hc control registers */
	regs_t *io;
	ret = pio_enable(regs, sizeof(regs_t), (void**)&io);
	CHECK_RET_FREE_INSTANCE("Failed to gain access to registers at %p.\n", io);
	instance->registers = io;
	uhci_print_verbose("Device registers accessible.\n");

	/* init transfer lists */
	ret = uhci_init_transfer_lists(instance->transfers);
	CHECK_RET_FREE_INSTANCE("Failed to initialize transfer lists.\n");
	uhci_print_verbose("Transfer lists initialized.\n");

	/* init root hub */
	ret = uhci_root_hub_init(&instance->root_hub, device,
	  (char*)regs + UHCI_ROOT_HUB_PORT_REGISTERS_OFFSET);
	CHECK_RET_FREE_INSTANCE("Failed to initialize root hub driver.\n");

	uhci_print_verbose("Initializing frame list.\n");
	instance->frame_list = get_page();
//	  memalign32(sizeof(link_pointer_t) * UHCI_FRAME_LIST_COUNT, 4096);
	if (instance->frame_list == NULL) {
		uhci_print_error("Failed to allocate frame list pointer.\n");
		uhci_root_hub_fini(&instance->root_hub);
		free(instance);
		return ENOMEM;
	}

	/* initialize all frames to point to the first queue head */
	unsigned i = 0;
	const uint32_t queue =
	  instance->transfers[USB_TRANSFER_INTERRUPT].queue_head_pa
	  | LINK_POINTER_QUEUE_HEAD_FLAG;
	for(; i < UHCI_FRAME_LIST_COUNT; ++i) {
		instance->frame_list[i] = queue;
	}

	const uintptr_t pa = (uintptr_t)addr_to_phys(instance->frame_list);

	pio_write_32(&instance->registers->flbaseadd, (uint32_t)pa);

	instance->cleaner = fibril_create(uhci_clean_finished, instance);
	fibril_add_ready(instance->cleaner);

	instance->debug_checker = fibril_create(uhci_debug_checker, instance);
	fibril_add_ready(instance->debug_checker);

	uhci_print_verbose("Starting UHCI HC.\n");
	pio_write_16(&instance->registers->usbcmd, UHCI_CMD_RUN_STOP);
/*
	uint16_t cmd = pio_read_16(&instance->registers->usbcmd);
	cmd |= UHCI_CMD_DEBUG;
	pio_write_16(&instance->registers->usbcmd, cmd);
*/
	device->driver_data = instance;
	return EOK;
}
/*----------------------------------------------------------------------------*/
int uhci_init_transfer_lists(transfer_list_t transfers[])
{
	//TODO:refactor
	transfers[USB_TRANSFER_ISOCHRONOUS].first = NULL;
	transfers[USB_TRANSFER_ISOCHRONOUS].last = NULL;

	int ret;
	ret = transfer_list_init(&transfers[USB_TRANSFER_BULK], NULL);
	if (ret != EOK) {
		uhci_print_error("Failed to initialize bulk queue.\n");
		return ret;
	}

	ret = transfer_list_init(
	  &transfers[USB_TRANSFER_CONTROL], &transfers[USB_TRANSFER_BULK]);
	if (ret != EOK) {
		uhci_print_error("Failed to initialize control queue.\n");
		transfer_list_fini(&transfers[USB_TRANSFER_BULK]);
		return ret;
	}

	ret = transfer_list_init(
	  &transfers[USB_TRANSFER_INTERRUPT], &transfers[USB_TRANSFER_CONTROL]);
	if (ret != EOK) {
		uhci_print_error("Failed to initialize interrupt queue.\n");
		transfer_list_fini(&transfers[USB_TRANSFER_CONTROL]);
		transfer_list_fini(&transfers[USB_TRANSFER_BULK]);
		return ret;
	}

	return EOK;
}
/*----------------------------------------------------------------------------*/
int uhci_transfer(
  device_t *dev,
  usb_target_t target,
  usb_transfer_type_t transfer_type,
	bool toggle,
  usb_packet_id pid,
  void *buffer, size_t size,
  usbhc_iface_transfer_out_callback_t callback_out,
  usbhc_iface_transfer_in_callback_t callback_in,
  void *arg )
{
	// TODO: Add support for isochronous transfers
	if (transfer_type == USB_TRANSFER_ISOCHRONOUS)
		return ENOTSUP;

	if (size >= 1024)
		return ENOTSUP;

	transfer_descriptor_t *td = NULL;
	callback_t *job = NULL;
	int ret = EOK;

#define CHECK_RET_TRANS_FREE_JOB_TD(message) \
	if (ret != EOK) { \
		uhci_print_error(message); \
		if (job) { \
			callback_dispose(job); \
		} \
		if (td) { free32(td); } \
		return ret; \
	} else (void) 0


	job = callback_get(dev, buffer, size, callback_in, callback_out, arg);
	ret = job ? EOK : ENOMEM;
	CHECK_RET_TRANS_FREE_JOB_TD("Failed to allocate callback structure.\n");

	td = transfer_descriptor_get(3, size, false, target, pid, job->new_buffer);
	ret = td ? EOK : ENOMEM;
	CHECK_RET_TRANS_FREE_JOB_TD("Failed to setup transfer descriptor.\n");

	td->callback = job;

	assert(dev);
	uhci_t *instance = (uhci_t*)dev->driver_data;
	assert(instance);

	uhci_print_verbose("Appending a new transfer to queue.\n");
	ret = transfer_list_append(&instance->transfers[transfer_type], td);
	CHECK_RET_TRANS_FREE_JOB_TD("Failed to append transfer descriptor.\n");

	return EOK;
}
/*---------------------------------------------------------------------------*/
int uhci_clean_finished(void* arg)
{
	uhci_print_verbose("Started cleaning fibril.\n");
	uhci_t *instance = (uhci_t*)arg;
	assert(instance);

	while(1) {
		uhci_print_verbose("Running cleaning fibril on: %p.\n", instance);
		/* iterate all transfer queues */
		int i = 0;
		for (; i < TRANSFER_QUEUES; ++i) {
			/* Remove inactive transfers from the top of the queue
			 * TODO: should I reach queue head or is this enough? */
			volatile transfer_descriptor_t * it =
				instance->transfers[i].first;
			uhci_print_verbose("Running cleaning fibril on queue: %p (%s).\n",
				&instance->transfers[i], it ? "SOMETHING" : "EMPTY");

		if (it)
			uhci_print_verbose("First in queue: %p (%x).\n",
				it, it->status);

			while (instance->transfers[i].first &&
			 !(instance->transfers[i].first->status & TD_STATUS_ERROR_ACTIVE)) {
				transfer_descriptor_t *transfer = instance->transfers[i].first;
				uhci_print_info("Inactive transfer calling callback with status %x.\n",
				  transfer->status);
				instance->transfers[i].first = transfer->next_va;
				transfer_descriptor_dispose(transfer);
			}
			if (!instance->transfers[i].first)
				instance->transfers[i].last = instance->transfers[i].first;
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
		uint16_t reg;
		reg = pio_read_16(&instance->registers->usbcmd);
		uhci_print_info("Command register: %X\n", reg);

		reg = pio_read_16(&instance->registers->usbsts);
		uhci_print_info("Status register: %X (%s,%s,%s,%s,%s,%s)\n",
		    reg,
		    UHCI_GET_STR_FLAG(reg, UHCI_STATUS_HALTED, "halted", "-"),
		    UHCI_GET_STR_FLAG(reg, UHCI_STATUS_PROCESS_ERROR, "prerr", "-"),
		    UHCI_GET_STR_FLAG(reg, UHCI_STATUS_SYSTEM_ERROR, "syserr", "-"),
		    UHCI_GET_STR_FLAG(reg, UHCI_STATUS_RESUME, "res", "-"),
		    UHCI_GET_STR_FLAG(reg, UHCI_STATUS_ERROR_INTERRUPT, "errintr", "-"),
		    UHCI_GET_STR_FLAG(reg, UHCI_STATUS_INTERRUPT, "intr", "-"));
/*
		uintptr_t frame_list = pio_read_32(&instance->registers->flbaseadd);
		uhci_print_verbose("Framelist address: %p vs. %p.\n",
			frame_list, addr_to_phys(instance->frame_list));
		int frnum = pio_read_16(&instance->registers->frnum) & 0x3ff;
		uhci_print_verbose("Framelist item: %d \n", frnum );

		queue_head_t* qh = instance->transfers[USB_TRANSFER_INTERRUPT].queue_head;
		uhci_print_verbose("Interrupt QH: %p vs. %p.\n",
			instance->frame_list[frnum], addr_to_phys(qh));

		uhci_print_verbose("Control QH: %p vs. %p.\n", qh->next_queue,
			addr_to_phys(instance->transfers[USB_TRANSFER_CONTROL].queue_head));
		qh = instance->transfers[USB_TRANSFER_CONTROL].queue_head;

		uhci_print_verbose("Bulk QH: %p vs. %p.\n", qh->next_queue,
			addr_to_phys(instance->transfers[USB_TRANSFER_BULK].queue_head));
	uint16_t cmd = pio_read_16(&instance->registers->usbcmd);
	cmd |= UHCI_CMD_RUN_STOP;
	pio_write_16(&instance->registers->usbcmd, cmd);
*/

		async_usleep(UHCI_DEBUGER_TIMEOUT);
	}
	return 0;
}
