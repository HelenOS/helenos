#include <errno.h>
#include <usb/debug.h>
#include <usb/usb.h>

#include "debug.h"
#include "name.h"
#include "uhci.h"

int uhci_init(device_t *device, void *regs)
{
	assert( device );
	uhci_print_info( "Initializing device at address %p\n", device);

	/* create instance */
	uhci_t *instance = malloc( sizeof(uhci_t) );
	if (!instance)
		{ return ENOMEM; }
	memset( instance, 0, sizeof(uhci_t) );

	/* init address keeper(libusb) */
	usb_address_keeping_init( &instance->address_manager, USB11_ADDRESS_MAX );

	/* allow access to hc control registers */
	regs_t *io;
	int ret = pio_enable( regs, sizeof(regs_t), (void**)&io);
	if (ret < 0) {
		free( instance );
		printf(NAME": Failed to gain access to registers at %p\n", io);
		return ret;
	}
	instance->registers = io;

	/* init root hub */
	ret = uhci_root_hub_init( &instance->root_hub, device,
	  (char*)regs + UHCI_ROOT_HUB_PORT_REGISTERS_OFFSET );
	if (ret < 0) {
		free( instance );
		printf(NAME": Failed to initialize root hub driver.\n");
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

	callback( dev, 0, USB_OUTCOME_OK, arg );

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

	callback( dev, USB_OUTCOME_OK, arg );
	return EOK;
}
/*----------------------------------------------------------------------------*/
