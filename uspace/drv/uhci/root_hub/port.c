
#include <errno.h>
#include <usb/usb.h>    /* usb_address_t */
#include <usb/usbdrv.h> /* usb_drv_*     */

#include "debug.h"
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
		uhci_print_verbose("Port(%d) status address %p:\n",
		  port_instance->number, port_instance->address);

		/* read register value */
		port_status_t port_status =
			port_status_read(port_instance->address);

		/* debug print */
		uhci_print_info("Port(%d) status %#.4x\n",
		  port_instance->number, port_status);
		print_port_status(port_status);

		if (port_status & STATUS_CONNECTED_CHANGED) {
			if (port_status & STATUS_CONNECTED) {
				/* new device */
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
	assert(port->hc_phone);

	uhci_print_info("Adding new device on port %d.\n", port->number);


	/* get default address */
	int ret = usb_drv_reserve_default_address(port->hc_phone);
	if (ret != EOK) {
		uhci_print_error("Failed to reserve default address.\n");
		return ret;
	}

	const usb_address_t usb_address = usb_drv_request_address(port->hc_phone);

	if (usb_address <= 0) {
		uhci_print_error("Recieved invalid address(%d).\n", usb_address);
		return usb_address;
	}
	/*
	 * the host then waits for at least 100 ms to allow completion of
	 * an insertion process and for power at the device to become stable.
	 */
	async_usleep(100000);

	/* enable port */
	uhci_port_set_enabled(port, true);

	/* The hub maintains the reset signal to that port for 10 ms
	 * (See Section 11.5.1.5)
	 */
	port_status_t port_status =
		port_status_read(port->address);
	port_status |= STATUS_IN_RESET;
	port_status_write(port->address, port_status);
	async_usleep(10000);
	port_status =
		port_status_read(port->address);
	port_status &= ~STATUS_IN_RESET;
	port_status_write(port->address, port_status);

	/* assign address to device */
	ret = usb_drv_req_set_address(port->hc_phone, 0, usb_address);


	if (ret != EOK) { /* address assigning went wrong */
		uhci_print_error("Failed(%d) to assign address to the device.\n", ret);
		uhci_port_set_enabled(port, false);
		int release = usb_drv_release_default_address(port->hc_phone);
		if (release != EOK) {
			uhci_print_fatal("Failed to release default address.\n");
			return release;
		}
		return ret;
	}

	/* release default address */
	ret = usb_drv_release_default_address(port->hc_phone);
	if (ret != EOK) {
		uhci_print_fatal("Failed to release default address.\n");
		return ret;
	}

	/* communicate and possibly report to devman */
	assert(port->attached_device == 0);

	ret = usb_drv_register_child_in_devman(port->hc_phone, port->hc, usb_address,
		&port->attached_device);

	if (ret != EOK) { /* something went wrong */
		uhci_print_error("Failed(%d) in usb_drv_register_child.\n", ret);
		uhci_port_set_enabled(port, false);
		return ENOMEM;
	}
	uhci_print_info("Sucessfully added device on port(%d) address(%d).\n",
		port->number, usb_address);

	/* TODO: bind the address here */

	return EOK;
}
/*----------------------------------------------------------------------------*/
static int uhci_port_remove_device(uhci_port_t *port)
{
	uhci_print_error("Don't know how to remove device %#x.\n",
		(unsigned int)port->attached_device);
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
	port_status_write(port->address, port_status);

	uhci_print_info("%s port %d.\n",
	  enabled ? "Enabled" : "Disabled", port->number);
	return EOK;
}
/*----------------------------------------------------------------------------*/
