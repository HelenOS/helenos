
#include <atomic.h>
#include <errno.h>
#include <usb/devreq.h>
#include <usb/usb.h>

#include "debug.h"
#include "uhci.h"
#include "port.h"
#include "port_status.h"
#include "utils/hc_synchronizer.h"

struct usb_match {
	int id_score;
	const char *id_string;
};

static int uhci_port_new_device(uhci_port_t *port);
static int uhci_port_set_enabled(uhci_port_t *port, bool enabled);
static usb_address_t assign_address_to_zero_device( device_t *hc );
static int report_new_device(
  device_t *hc, usb_address_t address, int hub_port, devman_handle_t *handle);

/*----------------------------------------------------------------------------*/
int uhci_port_check(void *port)
{
	uhci_port_t *port_instance = port;
	assert(port_instance);

	while (1) {
		uhci_print_info("Port(%d) status address %p:\n",
		  port_instance->number, port_instance->address);

		/* read register value */
		port_status_t port_status;
		port_status.raw_value = pio_read_16(port_instance->address);

		/* debug print */
		uhci_print_info("Port(%d) status %#x:\n",
		  port_instance->number, port_status.raw_value);
		print_port_status( &port_status );

		if (port_status.status.connect_change) {
			if (port_status.status.connected) {
				/* assign address and report new device */
				uhci_port_new_device(port_instance);
			} else {
				/* TODO */
				/* remove device here */
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

	/* release default address */
	usb_address_keeping_release_default(&uhci_instance->address_manager);

	if (address <= 0) { /* address assigning went wrong */
		uhci_port_set_enabled(port, false);
		uhci_print_error("Failed to assign address to the device");
		return ENOMEM;
	}

	/* report to devman */
	devman_handle_t child = 0;
	report_new_device(port->hc, address, port->number, &child);

	/* bind address */
	usb_address_keeping_devman_bind(&uhci_instance->address_manager,
	  address, child);

	return EOK;
}
/*----------------------------------------------------------------------------*/
static int uhci_port_set_enabled(uhci_port_t *port, bool enabled)
{
	assert(port);

	/* read register value */
	port_status_t port_status;
	port_status.raw_value = pio_read_16( port->address );

	/* enable port: register write */
	port_status.status.enabled_change = 0;
	port_status.status.enabled = (bool)enabled;
	pio_write_16( port->address, port_status.raw_value );

	uhci_print_info( "Enabled port %d.\n", port->number );
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int report_new_device(
  device_t *hc, usb_address_t address, int hub_port, devman_handle_t *handle)
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
		uhci_print_verbose( "Adding match id rule:%s\n", match_str );

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

	sync_value_t value;
	sync_init(&value);

	uhci_setup(
	  hc, new_device, USB_TRANSFER_CONTROL, &data, sizeof(data),
		sync_out_callback, (void*)&value );
	uhci_print_verbose("address assignment sent, waiting to complete.\n");

	sync_wait_for(&value);

	uhci_print_info( "Assigned address %#x.\n", usb_address );

	return usb_address;
}
