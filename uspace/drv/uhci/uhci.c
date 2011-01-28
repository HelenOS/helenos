#include <errno.h>
#include <usb/debug.h>
#include <usb/usb.h>

#include "translating_malloc.h"

#include "debug.h"
#include "name.h"
#include "uhci.h"

static int uhci_init_transfer_lists(transfer_list_t list[]);
static int uhci_clean_finished(void *arg);
static inline int uhci_add_transfer(
  device_t *dev,
  usb_target_t target,
  usb_transfer_type_t transfer_type,
  usb_packet_id pid,
  void *buffer, size_t size,
  usbhc_iface_transfer_out_callback_t callback_out,
  usbhc_iface_transfer_in_callback_t callback_in,
  void *arg );

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
	uhci_t *instance = malloc( sizeof(uhci_t) );
	int ret = instance ? EOK : ENOMEM;
	CHECK_RET_FREE_INSTANCE("Failed to allocate uhci driver instance.\n");

	bzero(instance, sizeof(uhci_t));

	/* init address keeper(libusb) */
	usb_address_keeping_init(&instance->address_manager, USB11_ADDRESS_MAX);

	/* allow access to hc control registers */
	regs_t *io;
	ret = pio_enable(regs, sizeof(regs_t), (void**)&io);
	CHECK_RET_FREE_INSTANCE("Failed to gain access to registers at %p.\n", io);
	instance->registers = io;

	/* init transfer lists */
	ret = uhci_init_transfer_lists(instance->transfers);
	CHECK_RET_FREE_INSTANCE("Failed to initialize transfer lists.\n");

	/* init root hub */
	ret = uhci_root_hub_init(&instance->root_hub, device,
	  (char*)regs + UHCI_ROOT_HUB_PORT_REGISTERS_OFFSET);
	CHECK_RET_FREE_INSTANCE("Failed to initialize root hub driver.\n");

	instance->frame_list =
	  trans_malloc(sizeof(link_pointer_t) * UHCI_FRAME_LIST_COUNT);
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

	device->driver_data = instance;
	return EOK;
}
/*----------------------------------------------------------------------------*/
int uhci_in(
  device_t *dev,
	usb_target_t target,
	usb_transfer_type_t transfer_type,
	void *buffer, size_t size,
	usbhc_iface_transfer_in_callback_t callback, void *arg
	)
{
	uhci_print_info( "transfer IN [%d.%d (%s); %zu]\n",
	    target.address, target.endpoint,
	    usb_str_transfer_type(transfer_type),
	    size);
	return uhci_add_transfer(
	  dev, target, transfer_type, USB_PID_IN, buffer, size, NULL, callback, arg);
}
/*----------------------------------------------------------------------------*/
int uhci_out(
  device_t *dev,
  usb_target_t target,
  usb_transfer_type_t transfer_type,
  void *buffer, size_t size,
	usbhc_iface_transfer_out_callback_t callback, void *arg
  )
{
	uhci_print_info( "transfer OUT [%d.%d (%s); %zu]\n",
	    target.address, target.endpoint,
	    usb_str_transfer_type(transfer_type),
	    size);
	return uhci_add_transfer(
	  dev, target, transfer_type, USB_PID_OUT, buffer, size, callback, NULL, arg);
}
/*----------------------------------------------------------------------------*/
int uhci_setup(
  device_t *dev,
  usb_target_t target,
  usb_transfer_type_t transfer_type,
  void *buffer, size_t size,
  usbhc_iface_transfer_out_callback_t callback, void *arg
  )
{
	uhci_print_info( "transfer SETUP [%d.%d (%s); %zu]\n",
	    target.address, target.endpoint,
	    usb_str_transfer_type(transfer_type),
	    size);
	uhci_print_info("Setup packet content: %x %x.\n", ((uint8_t*)buffer)[0],
	  ((uint8_t*)buffer)[1]);
	return uhci_add_transfer( dev,
	  target, transfer_type, USB_PID_SETUP, buffer, size, callback, NULL, arg);

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
static inline int uhci_add_transfer(
  device_t *dev,
  usb_target_t target,
  usb_transfer_type_t transfer_type,
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
			callback_fini(job); \
			trans_free(job); \
		} \
		if (td) { trans_free(td); } \
		return ret; \
	} else (void) 0


	job = malloc(sizeof(callback_t));
	ret= job ? EOK : ENOMEM;
	CHECK_RET_TRANS_FREE_JOB_TD("Failed to allocate callback structure.\n");

	ret = callback_init(job, dev, buffer, size, callback_in, callback_out, arg);
	CHECK_RET_TRANS_FREE_JOB_TD("Failed to initialize callback structure.\n");

	td = transfer_descriptor_get(3, size, false, target, pid);
	ret = td ? EOK : ENOMEM;
	CHECK_RET_TRANS_FREE_JOB_TD("Failed to setup transfer descriptor.\n");

	td->callback = job;

	assert(dev);
	uhci_t *instance = (uhci_t*)dev->driver_data;
	assert(instance);

	ret = transfer_list_append(&instance->transfers[transfer_type], td);
	CHECK_RET_TRANS_FREE_JOB_TD("Failed to append transfer descriptor.\n");

	return EOK;
}
/*----------------------------------------------------------------------------*/
int uhci_clean_finished(void* arg)
{
	uhci_print_verbose("Started cleaning fibril.\n");
	uhci_t *instance = (uhci_t*)arg;
	assert(instance);

	while(1) {
		uhci_print_verbose("Running cleaning fibril on %p.\n", instance);
		/* iterate all transfer queues */
		usb_transfer_type_t i = USB_TRANSFER_BULK;
		for (; i > USB_TRANSFER_ISOCHRONOUS; --i) {
			/* Remove inactive transfers from the top of the queue
			 * TODO: should I reach queue head or is this enough? */
			while (instance->transfers[i].first &&
			 !(instance->transfers[i].first->status & TD_STATUS_ERROR_ACTIVE)) {
				transfer_descriptor_t *transfer = instance->transfers[i].first;
				uhci_print_verbose("Cleaning fibril found inactive transport.");
				instance->transfers[i].first = transfer->next_va;
				transfer_descriptor_fini(transfer);
				trans_free(transfer);
			}
			if (!instance->transfers[i].first)
				instance->transfers[i].last = instance->transfers[i].first;
		}

		async_usleep(1000000);
	}
	return EOK;
}
