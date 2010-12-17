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

static int uhci_root_hub_check_ports( void * device );
/*----------------------------------------------------------------------------*/
int uhci_root_hub_init( uhci_root_hub_t *hub, device_t *device, void *addr )
{
	assert( hub );
	hub->checker = fibril_create( uhci_root_hub_check_ports, device );
	if (hub->checker == 0) {
		printf( NAME": Failed to launch root hub fibril." );
		return ENOMEM;
	}
	fibril_add_ready( hub->checker );
	port_regs_t *regs;
	const int ret = pio_enable( addr, sizeof(port_regs_t), (void**)&regs);
	if (ret < 0) {
		printf(NAME": Failed to gain access to port registers at %p\n", regs);
		return ret;
	}
	hub->registers = regs;

	return EOK;
}
/*----------------------------------------------------------------------------*/
int uhci_root_hub_fini( uhci_root_hub_t* instance )
{
	assert( instance );
	//destroy fibril here
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
			uint16_t value = pio_read_16( address );
			usb_dprintf( NAME, 1, "Port(%d) status 0x%x:\n", i, value );
			print_port_status( (port_status_t*)&value );
		}
		async_usleep( 1000000 );
	}
	return ENOTSUP;
}
