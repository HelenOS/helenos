/*
 * Copyright (c) 2010 Lenka Trochtova
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @defgroup pciintel pci bus driver for intel method 1.
 * @brief HelenOS root pci bus driver for intel method 1.
 * @{
 */

/** @file
 */

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <bool.h>
#include <fibril_synch.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <macros.h>

#include <driver.h>
#include <devman.h>
#include <ipc/devman.h>
#include <ipc/dev_iface.h>
#include <resource.h>
#include <device/hw_res.h>
#include <ddi.h>

#define NAME "pciintel"

static bool pci_add_device(device_t *dev);

/** The pci bus driver's standard operations.
 */
static driver_ops_t pci_ops = {
	.add_device = &pci_add_device
};

/** The pci bus driver structure. 
 */
static driver_t pci_driver = {
	.name = NAME,
	.driver_ops = &pci_ops
};

typedef struct pciintel_bus_data {
	void *conf_io_addr;
	void *conf_data_port;
	void *conf_addr_port;	
} pci_bus_data_t;


static bool pci_add_device(device_t *dev)
{
	printf(NAME ": pci_add_device\n");
	
	pci_bus_data_t *bus_data = (pci_bus_data_t *)malloc(sizeof(pci_bus_data_t));
	if (NULL == bus_data) {
		printf(NAME ": pci_add_device allocation failed.\n");
		return false;
	}	
	
	dev->parent_phone = devman_parent_device_connect(dev->handle,  IPC_FLAG_BLOCKING);
	if (dev->parent_phone <= 0) {
		printf(NAME ": pci_add_device failed to connect to the parent's driver.\n");
		free(bus_data);
		return false;
	}
	
	hw_resource_list_t hw_resources;
	
	if (!get_hw_resources(dev->parent_phone, &hw_resources)) {
		printf(NAME ": pci_add_device failed to get hw resources for the device.\n");
		free(bus_data);
		ipc_hangup(dev->parent_phone);
		return false;		
	}
	
	
	printf(NAME ": conf_addr = %x.\n", hw_resources.resources[0].res.reg.address);	
	
	assert(hw_resources.count > 0);
	assert(hw_resources.resources[0].type == REGISTER);
	assert(hw_resources.resources[0].res.reg.size == 8);
	
	bus_data->conf_io_addr = hw_resources.resources[0].res.reg.address;
	
	if (pio_enable(bus_data->conf_io_addr, 8, &bus_data->conf_addr_port)) {
		printf(NAME ": failed to enable configuration ports.\n");
		free(bus_data);
		ipc_hangup(dev->parent_phone);
		clean_hw_resource_list(&hw_resources);
		return false;					
	}
	bus_data->conf_data_port = (char *)bus_data->conf_addr_port + 4;
	
	dev->driver_data = bus_data;
	
	// TODO scan the bus	
	
	clean_hw_resource_list(&hw_resources);
	
	return true;
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS pci bus driver (intel method 1).\n");	
	return driver_main(&pci_driver);
}

/**
 * @}
 */