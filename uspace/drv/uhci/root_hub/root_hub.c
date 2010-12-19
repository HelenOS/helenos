#include <bool.h>
#include <ddi.h>
#include <devman.h>
#include <async.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include "../name.h"
#include "../uhci.h"
#include "../debug.h"
#include "port_status.h"
#include "root_hub.h"

#define ROOT_HUB_WAIT_USEC 10000000 /* 10 second */

struct usb_match {
	int id_score;
	const char *id_string;
};

static int uhci_root_hub_check_ports( void *hc );
static int uhci_root_hub_new_device( device_t *hc, unsigned port );
static usb_address_t uhci_root_hub_assign_address( device_t *hc );
static int uhci_root_hub_report_new_device(
  device_t *hc, usb_address_t address, int port, devman_handle_t *handle );
static int uhci_root_hub_port_set_enabled(
  uhci_root_hub_t *instance, unsigned port, bool enabled );

/*----------------------------------------------------------------------------*/
int uhci_root_hub_init( uhci_root_hub_t *hub, device_t *hc, void *addr )
{
	assert( hub );

	/* allow access to root hub registers */
	port_regs_t *regs;
	const int ret = pio_enable( addr, sizeof(port_regs_t), (void**)&regs);
	if (ret < 0) {
		uhci_print_error(
		  ": Failed to gain access to port registers at %p\n", regs );
		return ret;
	}
	hub->registers = regs;

	/* add fibril for periodic checks */
	hub->checker = fibril_create( uhci_root_hub_check_ports, hc );
	if (hub->checker == 0) {
		uhci_print_error( ": failed to launch root hub fibril." );
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
static int uhci_root_hub_port_set_enabled( uhci_root_hub_t *instance,
  unsigned port, bool enabled )
{
	assert( instance );
	assert( instance->registers );
	assert( port < UHCI_ROOT_HUB_PORT_COUNT );

	volatile uint16_t * address =
		&(instance->registers->portsc[port]);

	/* read register value */
	port_status_t port_status;
	port_status.raw_value = pio_read_16( address );

	/* enable port: register write */
	port_status.status.enabled_change = 0;
	port_status.status.enabled = (bool)enabled;
	pio_write_16( address, port_status.raw_value );

	uhci_print_info( "Enabled port %d.\n", port );
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

			uhci_print_info( "Port(%d) status address %p:\n", i, address );

			/* read register value */
			port_status_t port_status;
			port_status.raw_value = pio_read_16( address );

			/* debug print */
			uhci_print_info( "Port(%d) status %#x:\n", i, port_status.raw_value );
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

	uhci_print_info( "Adding new device on port %d.\n", port );

	uhci_t *uhci_instance = (uhci_t*)hc->driver_data;

	/* get default address */
	usb_address_keeping_reserve_default( &uhci_instance->address_manager );

	/* enable port */
	uhci_root_hub_port_set_enabled( &uhci_instance->root_hub, port, true );

	/* assign address to device */
	usb_address_t address = uhci_root_hub_assign_address( hc );

	/* release default address */
	usb_address_keeping_release_default( &uhci_instance->address_manager );

	if (address <= 0) { /* address assigning went wrong */
		uhci_root_hub_port_set_enabled( &uhci_instance->root_hub, port, false );
		uhci_print_error( "Failed to assign address to the device" );
		return ENOMEM;
	}

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
	const usb_address_t usb_address =
	  usb_address_keeping_request( &uhci_instance->address_manager );

	/* assign new address */
	/* TODO send new address to the device*/
	uhci_print_error( "Assigned address %#x.\n", usb_address );

	return usb_address;
}
/*----------------------------------------------------------------------------*/
static int uhci_root_hub_report_new_device(
  device_t *hc, usb_address_t address, int hub_port, devman_handle_t *handle )
{
	assert( hc );
	assert( address > 0 );
	assert( address <= USB11_ADDRESS_MAX );

	device_t *child = create_device();
	if (child == NULL)
		{ return ENOMEM; }

	char *name;
	int ret;

	ret = asprintf( &name, "usbdevice on hc%p/%d/%#x", hc, hub_port, address );
	if (ret < 0) {
		uhci_print_error( "Failed to create device name.\n" );
		delete_device( child );
		return ret;
	}
	child->name = name;

	/* TODO get and parse device descriptor */
	const int vendor = 1;
	const int product = 1;
	const char* release = "unknown";
	const char* class = "unknown";

	/* create match ids TODO fix class printf*/
	static const struct usb_match usb_matches[] = {
	  { 100, "usb&vendor=%d&product=%d&release=%s" },
	  {  90, "usb&vendor=%d&product=%d" },
	  {  50, "usb&class=%d" },
	  {   1, "usb&fallback" }
	};

	unsigned i = 0;
	for (;i < sizeof( usb_matches )/ sizeof( struct usb_match ); ++i ) {
		char *match_str;
		const int ret = asprintf(
		  &match_str, usb_matches[i].id_string, vendor, product, release, class );
		if (ret < 0 ) {
			uhci_print_error( "Failed to create matchid string.\n" );
			delete_device( child );
			return ret;
		}
		uhci_print_info( "Adding match id rule:%s\n", match_str );

		match_id_t *id = create_match_id();
		if (id == NULL) {
			uhci_print_error( "Failed to create matchid.\n" );
			delete_device( child );
			free( match_str );
			return ENOMEM;
		}
		id->id = match_str;
		id->score = usb_matches[i].id_score;
		add_match_id( &child->match_ids, id );

		uhci_print_info( "Added match id, score: %d, string %s\n",
		  id->score, id->id );
	}

	ret = child_device_register( child, hc );
	if (ret < 0) {
		uhci_print_error( "Failed to create device name.\n" );
		delete_device( child );
		return ret;
	}

	if (handle != NULL)
		{ *handle = child->handle; }

	return EOK;
}
