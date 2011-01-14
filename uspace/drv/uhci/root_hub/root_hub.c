#include <async.h>
#include <ddi.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include "debug.h"
#include "root_hub.h"

#define ROOT_HUB_WAIT_USEC 10000000 /* 10 seconds */

int uhci_root_hub_init( uhci_root_hub_t *hub, device_t *hc, void *addr )
{
	assert( hub );

	/* allow access to root hub registers */
	port_status_t *regs;
	const int ret = pio_enable(
	  addr, sizeof(port_status_t) * UHCI_ROOT_HUB_PORT_COUNT, (void**)&regs);

	if (ret < 0) {
		uhci_print_error(": Failed to gain access to port registers at %p\n", regs);
		return ret;
	}

	/* add fibrils for periodic port checks */
	unsigned i = 0;
	for (; i < UHCI_ROOT_HUB_PORT_COUNT; ++i) {
		/* mind pointer arithmetics */
		uhci_port_init(
		  &hub->ports[i], regs + i, hc, i, ROOT_HUB_WAIT_USEC);

		hub->checker[i] = fibril_create(uhci_port_check, &hub->ports[i]);
		if (hub->checker[i] == 0) {
			uhci_print_error(": failed to launch root hub fibril.");
			return ENOMEM;
		}
		fibril_add_ready(hub->checker[i]);
		uhci_print_verbose(" added fibril for port %d: %p.\n", i, hub->checker[i]);
	}

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
