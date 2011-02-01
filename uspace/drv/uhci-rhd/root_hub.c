#include <async.h>
#include <ddi.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include <usb/usbdrv.h>
#include <usb/debug.h>

#include "root_hub.h"


int uhci_root_hub_init(
  uhci_root_hub_t *instance, void *addr, size_t size, device_t *rh)
{
	assert(instance);
	assert(rh);
	int ret;
	ret = usb_drv_find_hc(rh, &instance->hc_handle);
	usb_log_info("rh found(%d) hc handle: %d.\n", ret, instance->hc_handle);
	if (ret != EOK) {
		return ret;
	}

	/* connect to the parent device (HC) */
	rh->parent_phone = devman_device_connect(8, 0);
	//usb_drv_hc_connect(rh, instance->hc_handle, 0);
	if (rh->parent_phone < 0) {
		usb_log_error("Failed to connect to the HC device.\n");
		return rh->parent_phone;
	}

	/* allow access to root hub registers */
	assert(sizeof(port_status_t) * UHCI_ROOT_HUB_PORT_COUNT == size);
	port_status_t *regs;
	ret = pio_enable(
	  addr, sizeof(port_status_t) * UHCI_ROOT_HUB_PORT_COUNT, (void**)&regs);

	if (ret < 0) {
		usb_log_error("Failed to gain access to port registers at %p\n", regs);
		return ret;
	}

	/* add fibrils for periodic port checks */
	unsigned i = 0;
	for (; i < UHCI_ROOT_HUB_PORT_COUNT; ++i) {
		/* mind pointer arithmetics */
		int ret = uhci_port_init(
		  &instance->ports[i], regs + i, i, ROOT_HUB_WAIT_USEC, rh);
		if (ret != EOK) {
			unsigned j = 0;
			for (;j < i; ++j)
				uhci_port_fini(&instance->ports[j]);
			return ret;
		}
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
