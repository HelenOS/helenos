#include <ddi.h>
#include <async.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include <usb/debug.h>

#include "../name.h"
#include "../uhci.h"
#include "port_status.h"
#include "root_hub.h"

#define ROOT_HUB_WAIT_USEC 10000000 /* 10 second */

static int uhci_root_hub_check_ports( void *hc );
static int uhci_root_hub_new_device( device_t *hc, unsigned port );
static usb_address_t uhci_root_hub_assign_address( device_t *hc );
static int uhci_root_hub_report_new_device(
  device_t *hc, usb_address_t address, int port, devman_handle_t *handle );

/*----------------------------------------------------------------------------*/
int uhci_root_hub_init( uhci_root_hub_t *hub, device_t *hc, void *addr )
{
	assert( hub );

	/* allow access to root hub registers */
	port_regs_t *regs;
	const int ret = pio_enable( addr, sizeof(port_regs_t), (void**)&regs);
	if (ret < 0) {
		printf( NAME": Failed to gain access to port registers at %p\n", regs );
		return ret;
	}
	hub->registers = regs;

	/* add fibril for periodic checks */
	hub->checker = fibril_create( uhci_root_hub_check_ports, hc );
	if (hub->checker == 0) {
		printf( NAME": failed to launch root hub fibril." );
		return ENOMEM;
	}
	fibril_add_ready( hub->checker );

	return EOK;
}
/*----------------------------------------------------------------------------*/
int uhci_root_hub_fini( uhci_root_hub_t* instance )
{
	assert( instance );
	// TODO:
	//destroy fibril here
	//disable access to registers
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int uhci_root_hub_check_ports( void * device )
{
	assert( device );
	uhci_t *uhci_instance = ((device_t*)device)->driver_data;

	while (1) {
		int i = 0;
		for (; i < UHCI_ROOT_HUB_PORT_COUNT; ++i) {
			volatile uint16_t * address =
				&(uhci_instance->root_hub.registers->portsc[i]);

			usb_dprintf( NAME, 1, "Port(%d) status address %p:\n", i, address );

			/* read register value */
			port_status_t port_status;
			port_status.raw_value = pio_read_16( address );

			/* debug print */
			usb_dprintf( NAME, 1, "Port(%d) status 0x%x:\n", i, port_status.raw_value );
			print_port_status( &port_status );

			if (port_status.status.connect_change) {
				if (port_status.status.connected) {
					/* assign address and report new device */
					uhci_root_hub_new_device( (device_t*)device, i );
				} else {
					/* TODO */
					/* remove device here */
				}
			}
		}
		async_usleep( ROOT_HUB_WAIT_USEC );
	}
	return ENOTSUP;
}
/*----------------------------------------------------------------------------*/
int uhci_root_hub_new_device( device_t *hc, unsigned port )
{
	assert( hc );
	assert( hc->driver_data );
	assert( port < UHCI_ROOT_HUB_PORT_COUNT );

	usb_dprintf( NAME, 2, "Adding new device on port %d.\n", port );

	uhci_t *uhci_instance = (uhci_t*)hc->driver_data;

	/* get default address */
	usb_address_keeping_reserve_default( &uhci_instance->address_manager );

	/* enable port */
	{
		volatile uint16_t * address =
			&(uhci_instance->root_hub.registers->portsc[port]);

		/* read register value */
		port_status_t port_status;
		port_status.raw_value = pio_read_16( address );

		/* enable port: register write */
		port_status.status.enabled = 1;
		pio_write_16( address, port_status.raw_value );

		usb_dprintf( NAME, 2, "Enabled port %d.\n", port );
	}

	/* assign address to device */
	usb_address_t address =
	 uhci_root_hub_assign_address( hc );
	if (address <= 0) {
		printf( NAME": Failed to assign address to the device" );
		return ENOMEM;
	}
	/* release default address */
	usb_address_keeping_release_default( &uhci_instance->address_manager );

	/* report to devman */
	devman_handle_t child = 0;
	uhci_root_hub_report_new_device( hc, address, port, &child );

	/* bind address */
	usb_address_keeping_devman_bind( &uhci_instance->address_manager,
	  address, child );


	return EOK;
}
/*----------------------------------------------------------------------------*/
static usb_address_t uhci_root_hub_assign_address( device_t *hc )
{
	assert( hc );
	assert( hc->driver_data );

	uhci_t *uhci_instance = (uhci_t*)hc->driver_data;
	/* get new address */
	const usb_address_t usb_address = usb_address_keeping_request(
	  &uhci_instance->address_manager );

	/* assign new address */
	/* TODO send new address*/
	usb_dprintf( NAME, 3, "Assigned address 0x%x.\n", usb_address );

	return usb_address;
}
/*----------------------------------------------------------------------------*/
static int uhci_root_hub_report_new_device(
  device_t *hc, usb_address_t address, int hub_port, devman_handle_t *handle )
{
	assert( hc );
	assert( address > 0 );
	assert( address <= USB11_ADDRESS_MAX );

	int ret;
	device_t *child = create_device();
	if (child == NULL)
		{ return ENOMEM; }
	char *name;

	ret = asprintf( &name, "usbdevice on hc%p/%d/0x%x", hc, hub_port, address );
	if (ret < 0) {
		usb_dprintf( NAME, 4, "Failed to create device name.\n" );
		delete_device( child );
		return ret;
	}
	child->name = name;

	/* TODO create match ids */

	ret = child_device_register( child, hc );
	if (ret < 0) {
		usb_dprintf( NAME, 4, "Failed to create device name.\n" );
		delete_device( child );
		return ret;
	}

	if (handle != NULL)
		{ *handle = child->handle; }

	return EOK;
}
