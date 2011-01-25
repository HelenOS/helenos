#include <errno.h>
#include <usb/debug.h>
#include <usb/usb.h>

#include "translating_malloc.h"

#include "debug.h"
#include "name.h"
#include "uhci.h"

static int init_tranfer_lists(transfer_list_t list[]);

//static int init_transfer();

int uhci_init(device_t *device, void *regs)
{
	assert( device );
	uhci_print_info( "Initializing device at address %p\n", device);

	/* create instance */
	uhci_t *instance = malloc( sizeof(uhci_t) );
	if (!instance)
		{ return ENOMEM; }
	bzero( instance, sizeof(uhci_t) );

	/* init address keeper(libusb) */
	usb_address_keeping_init( &instance->address_manager, USB11_ADDRESS_MAX );

	/* allow access to hc control registers */
	regs_t *io;
	int ret = pio_enable( regs, sizeof(regs_t), (void**)&io);
	if (ret < 0) {
		free( instance );
		uhci_print_error("Failed to gain access to registers at %p\n", io);
		return ret;
	}
	instance->registers = io;

	/* init root hub */
	ret = uhci_root_hub_init(&instance->root_hub, device,
	  (char*)regs + UHCI_ROOT_HUB_PORT_REGISTERS_OFFSET);
	if (ret < 0) {
		free(instance);
		uhci_print_error("Failed to initialize root hub driver.\n");
		return ret;
	}

	instance->frame_list = trans_malloc(sizeof(frame_list_t));
	if (instance->frame_list == NULL) {
		uhci_print_error("Failed to allocate frame list pointer.\n");
		uhci_root_hub_fini(&instance->root_hub);
		free(instance);
		return ENOMEM;
	}

	const uintptr_t pa = (uintptr_t)addr_to_phys(instance->frame_list);

	pio_write_32(&instance->registers->flbaseadd, (uint32_t)pa);

	ret = init_tranfer_lists(instance->transfers);
	if (ret != EOK) {
		uhci_print_error("Transfer list initialization failed.\n");
		uhci_root_hub_fini(&instance->root_hub);
		free(instance);
		return ret;
	}

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

	ret = callback_in_init(job, dev, buffer, size, callback, arg);
	CHECK_RET_TRANS_FREE_JOB_TD("Failed to initialize callback structure.\n");

	td = trans_malloc(sizeof(transfer_descriptor_t));
	ret = td ? ENOMEM : EOK;
	CHECK_RET_TRANS_FREE_JOB_TD("Failed to allocate tranfer descriptor.\n");

	ret = transfer_descriptor_init(td, 3, size, false, target, USB_PID_IN);
	CHECK_RET_TRANS_FREE_JOB_TD("Failed to initialize transfer descriptor.\n");

	td->callback = job;

	assert(dev);
	uhci_t *instance = (uhci_t*)dev->driver_data;
	assert(instance);

	ret = transfer_list_append(&instance->transfers[transfer_type], td);
	CHECK_RET_TRANS_FREE_JOB_TD("Failed to append transfer descriptor.\n");

	return EOK;
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

	callback( dev, USB_OUTCOME_OK, arg );
	return EOK;
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

	callback( dev, USB_OUTCOME_OK, arg );
	return EOK;
}
/*----------------------------------------------------------------------------*/
int init_tranfer_lists(transfer_list_t transfers[])
{
	//TODO:refactor
	transfers[USB_TRANSFER_ISOCHRONOUS].first = NULL;
	transfers[USB_TRANSFER_ISOCHRONOUS].last = NULL;
	int ret;
	ret = transfer_list_init(&transfers[USB_TRANSFER_BULK], NULL);
	if (ret != EOK) {
		uhci_print_error("Failed to inititalize bulk queue.\n");
		return ret;
	}

	ret = transfer_list_init(
	  &transfers[USB_TRANSFER_CONTROL], &transfers[USB_TRANSFER_BULK]);
	if (ret != EOK) {
		uhci_print_error("Failed to inititalize control queue.\n");
		transfer_list_fini(&transfers[USB_TRANSFER_BULK]);
		return ret;
	}

	ret = transfer_list_init(
	  &transfers[USB_TRANSFER_INTERRUPT], &transfers[USB_TRANSFER_CONTROL]);
	if (ret != EOK) {
		uhci_print_error("Failed to interrupt control queue.\n");
		transfer_list_fini(&transfers[USB_TRANSFER_CONTROL]);
		transfer_list_fini(&transfers[USB_TRANSFER_BULK]);
		return ret;
	}

	return EOK;
}
