
#include <errno.h>
//#include <usb/devreq.h> /* for usb_device_request_setup_packet_t */
#include <usb/usb.h>
#include <usb/usbdrv.h>

#include "debug.h"
#include "uhci.h"
#include "port.h"
#include "port_status.h"

static int uhci_port_new_device(uhci_port_t *port);
static int uhci_port_remove_device(uhci_port_t *port);
static int uhci_port_set_enabled(uhci_port_t *port, bool enabled);

/*----------------------------------------------------------------------------*/
int uhci_port_check(void *port)
{
	uhci_port_t *port_instance = port;
	assert(port_instance);
	port_instance->hc_phone = devman_device_connect(port_instance->hc->handle, 0);

	while (1) {
		uhci_print_info("Port(%d) status address %p:\n",
		  port_instance->number, port_instance->address);

		/* read register value */
		port_status_t port_status =
			port_status_read(port_instance->address);

		/* debug print */
		uhci_print_info("Port(%d) status %#.4x:\n",
		  port_instance->number, port_status);
		print_port_status( port_status );

		if (port_status & STATUS_CONNECTED_CHANGED) {
			if (port_status & STATUS_CONNECTED) {
				uhci_port_new_device(port_instance);
			} else {
				uhci_port_remove_device(port_instance);
			}
		}
		async_usleep(port_instance->wait_period_usec);
	}
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int uhci_port_new_device(uhci_port_t *port)
{
	assert(port);
	assert(port->hc);


	uhci_print_info("Adding new device on port %d.\n", port->number);

	uhci_t *uhci_instance = (uhci_t*)(port->hc->driver_data);

	/* get default address */
	usb_address_keeping_reserve_default(&uhci_instance->address_manager);

	const usb_address_t usb_address =
	  usb_address_keeping_request(&uhci_instance->address_manager);

	if (usb_address <= 0) {
		return usb_address;
	}

	/* enable port */
	uhci_port_set_enabled( port, true );

	/* assign address to device */
	int ret = usb_drv_req_set_address( port->hc_phone, 0, usb_address );


	if (ret != EOK) { /* address assigning went wrong */
		uhci_print_error("Failed(%d) to assign address to the device.\n", ret);
		uhci_port_set_enabled(port, false);
		usb_address_keeping_release_default(&uhci_instance->address_manager);
		return ENOMEM;
	}

	/* release default address */
	usb_address_keeping_release_default(&uhci_instance->address_manager);

	/* communicate and possibly report to devman */
	assert(port->attached_device == 0);

	ret = usb_drv_register_child_in_devman(port->hc_phone, port->hc, usb_address,
		&port->attached_device);

	if (ret != EOK) { /* something went wrong */
		uhci_print_error("Failed(%d) in usb_drv_register_child.\n", ret);
		uhci_port_set_enabled(port, false);
		return ENOMEM;
	}


	return EOK;
}
/*----------------------------------------------------------------------------*/
static int uhci_port_remove_device(uhci_port_t *port)
{
	uhci_print_error(	"Don't know how to remove device %#x.\n",
		port->attached_device);
	uhci_port_set_enabled(port, false);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int uhci_port_set_enabled(uhci_port_t *port, bool enabled)
{
	assert(port);

	/* read register value */
	port_status_t port_status
		= port_status_read(port->address);

	/* enable port: register write */
	if (enabled) {
		port_status |= STATUS_ENABLED;
	} else {
		port_status &= ~STATUS_ENABLED;
	}
	port_status_write( port->address, port_status );

	uhci_print_info( "%s port %d.\n",
	  enabled ? "Enabled" : "Disabled", port->number );
	return EOK;
}
/*----------------------------------------------------------------------------*/
