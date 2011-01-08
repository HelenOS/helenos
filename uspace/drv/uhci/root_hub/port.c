
#include <errno.h>
#include <usb/devreq.h> /* for usb_device_request_setup_packet_t */
#include <usb/usb.h>

#include "debug.h"
#include "uhci.h"
#include "port.h"
#include "port_status.h"
#include "utils/hc_synchronizer.h"
#include "utils/usb_device.h"

static int uhci_port_new_device(uhci_port_t *port);
static int uhci_port_remove_device(uhci_port_t *port);
static int uhci_port_set_enabled(uhci_port_t *port, bool enabled);
static usb_address_t assign_address_to_zero_device(device_t *hc);

/*----------------------------------------------------------------------------*/
int uhci_port_check(void *port)
{
	uhci_port_t *port_instance = port;
	assert(port_instance);

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

	/* enable port */
	uhci_port_set_enabled( port, true );

	/* assign address to device */
	usb_address_t address = assign_address_to_zero_device(port->hc);


	if (address <= 0) { /* address assigning went wrong */
		uhci_print_error("Failed to assign address to the device.\n");
		uhci_port_set_enabled(port, false);
		usb_address_keeping_release_default(&uhci_instance->address_manager);
		return ENOMEM;
	}

	/* release default address */
	usb_address_keeping_release_default(&uhci_instance->address_manager);

	/* communicate and possibly report to devman */
	assert(port->attached_device == 0);

#define CHECK_RET_DELETE_CHILD_RETURN(ret, child, message, args...)\
	if (ret < 0) { \
		uhci_print_error("Failed(%d) to "message, ret, ##args); \
		if (child) { \
			delete_device(child); \
		} \
		uhci_port_set_enabled(port, false); \
		usb_address_keeping_release(&uhci_instance->address_manager, address); \
		return ret; \
	} else (void)0

	device_t *child = create_device();

	int ret = child ? EOK : ENOMEM;
	CHECK_RET_DELETE_CHILD_RETURN(ret, child, "create device.\n" );

	ret = usb_device_init(child, port->hc, address, port->number );
	CHECK_RET_DELETE_CHILD_RETURN(ret, child, "init usb device.\n" );

	ret = child_device_register(child, port->hc);
	CHECK_RET_DELETE_CHILD_RETURN(ret, child, "register usb device.\n" );

	/* store device and bind address, can not fail */
	port->attached_device = child->handle;
	usb_address_keeping_devman_bind(&uhci_instance->address_manager,
	  address, port->attached_device);

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
static usb_address_t assign_address_to_zero_device( device_t *hc )
{
	assert( hc );
	assert( hc->driver_data );

	uhci_t *uhci_instance = (uhci_t*)hc->driver_data;

	/* get new address */
	const usb_address_t usb_address =
	  usb_address_keeping_request(&uhci_instance->address_manager);

	if (usb_address <= 0) {
		return usb_address;
	}

	/* assign new address */
	usb_target_t new_device = { USB_ADDRESS_DEFAULT, 0 };
	usb_device_request_setup_packet_t data =
	{
		.request_type = 0,
		.request = USB_DEVREQ_SET_ADDRESS,
		{ .value = usb_address },
		.index = 0,
		.length = 0
	};

	sync_value_t sync;
	uhci_setup_sync(hc, new_device,
	  USB_TRANSFER_CONTROL, &data, sizeof(data), &sync);

	if (sync.result != USB_OUTCOME_OK) {
		uhci_print_error(
		  "Failed to assign address to the connected device.\n");
		usb_address_keeping_release(&uhci_instance->address_manager,
		  usb_address);
		return -1;
	}

	uhci_print_info("Assigned address %#x.\n", usb_address);
	return usb_address;
}
