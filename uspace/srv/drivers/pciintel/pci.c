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
#include <libarch/ddi.h>

#include "pci.h"
#include "pci_regs.h"

#define NAME "pciintel"


#define CONF_ADDR(bus, dev, fn, reg)   ((1 << 31) | (bus << 16) | (dev << 11) | (fn << 8) | (reg & ~3))


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
	fibril_mutex_t conf_mutex;
} pci_bus_data_t;

static inline pci_bus_data_t *create_pci_bus_data() 
{
	pci_bus_data_t *bus_data = (pci_bus_data_t *)malloc(sizeof(pci_bus_data_t));
	if(NULL != bus_data) {
		memset(bus_data, 0, sizeof(pci_bus_data_t));
		fibril_mutex_initialize(&bus_data->conf_mutex);
	}
	return bus_data;	
}

static inline void delete_pci_bus_data(pci_bus_data_t *bus_data) 
{
	free(bus_data);	
}

static void pci_conf_read(device_t *dev, int reg, uint8_t *buf, size_t len)
{
	assert(NULL != dev->parent);
	
	pci_dev_data_t *dev_data = (pci_dev_data_t *)dev->driver_data;
	pci_bus_data_t *bus_data = (pci_bus_data_t *)dev->parent->driver_data;
	
	fibril_mutex_lock(&bus_data->conf_mutex);
	
	uint32_t conf_addr =  CONF_ADDR(dev_data->bus, dev_data->dev, dev_data->fn, reg);
	void *addr = bus_data->conf_data_port + (reg & 3);
	
	pio_write_32(bus_data->conf_addr_port, conf_addr);
	
	switch (len) {
		case 1:
			buf[0] = pio_read_8(addr);
			break;
		case 2:
			((uint16_t *)buf)[0] = pio_read_16(addr);
			break;
		case 4:
			((uint32_t *)buf)[0] = pio_read_32(addr);
			break;
	}
	
	fibril_mutex_unlock(&bus_data->conf_mutex);	
}

uint8_t pci_conf_read_8(device_t *dev, int reg)
{
	uint8_t res;
	pci_conf_read(dev, reg, &res, 1);
	return res;
}

uint16_t pci_conf_read_16(device_t *dev, int reg)
{
	uint16_t res;
	pci_conf_read(dev, reg, (uint8_t *)&res, 2);
	return res;
}

uint32_t pci_conf_read_32(device_t *dev, int reg)
{
	uint32_t res;
	pci_conf_read(dev, reg, (uint8_t *)&res, 4);
	return res;	
}

void create_pci_match_ids(device_t *dev)
{
	pci_dev_data_t *dev_data = (pci_dev_data_t *)dev->driver_data;
	match_id_t *match_id = NULL;	
	match_id = create_match_id();
	if (NULL != match_id) {
		asprintf(&match_id->id, "pci/ven=%04x,dev=%04x", dev_data->vendor_id, dev_data->device_id);
		match_id->score = 90;
		add_match_id(&dev->match_ids, match_id);
	}	
	// TODO add more ids (with subsys ids, using class id etc.)
}


void pci_bus_scan(device_t *parent, int bus_num) 
{
	device_t *dev = create_device();
	pci_dev_data_t *dev_data = create_pci_dev_data();
	dev->driver_data = dev_data;
	dev->parent = parent;
	
	int child_bus = 0;
	int dnum, fnum;
	bool multi;
	uint8_t header_type;
	
	for (dnum = 0; dnum < 32; dnum++) {
		multi = true;
		for (fnum = 0; multi && fnum < 8; fnum++) {
			init_pci_dev_data(dev_data, bus_num, dnum, fnum);
			dev_data->vendor_id = pci_conf_read_16(dev, PCI_VENDOR_ID);
			dev_data->device_id = pci_conf_read_16(dev, PCI_DEVICE_ID);
			if (dev_data->vendor_id == 0xFFFF) { // device is not present, go on scanning the bus
				if (fnum == 0) {
					break;
				} else {
					continue;  
				}
			}
			header_type = pci_conf_read_8(dev, PCI_HEADER_TYPE);
			if (fnum == 0) {
				 multi = header_type >> 7;  // is the device multifunction?
			}
			header_type = header_type & 0x7F; // clear the multifunction bit
			
			// TODO initialize device - interfaces, hw resources
			
			create_pci_dev_name(dev);
			printf(NAME ": adding new device name %s.\n", dev->name);
			
			create_pci_match_ids(dev);
			
			if (!child_device_register(dev, parent)) {
				clean_match_ids(&dev->match_ids);
				free((char *)dev->name);
				dev->name = NULL;
				continue;
			}
			
			if (header_type == PCI_HEADER_TYPE_BRIDGE || header_type == PCI_HEADER_TYPE_CARDBUS ) {
				child_bus = pci_conf_read_8(dev, PCI_BRIDGE_SEC_BUS_NUM);
				printf(NAME ": device is pci-to-pci bridge, secondary bus number = %d.\n", bus_num);
				if(child_bus > bus_num) {			
					pci_bus_scan(parent, child_bus);	
				}					
			}
			
			dev = create_device();  // alloc new aux. dev. structure
			dev_data = create_pci_dev_data();
			dev->driver_data = dev_data;
			dev->parent = parent;
		}
	}
	
	if (dev_data->vendor_id == 0xFFFF) {
		delete_device(dev);
		delete_pci_dev_data(dev_data);  // free the auxiliary device structure
	}	
	
}

static bool pci_add_device(device_t *dev)
{
	printf(NAME ": pci_add_device\n");
	
	pci_bus_data_t *bus_data = create_pci_bus_data();
	if (NULL == bus_data) {
		printf(NAME ": pci_add_device allocation failed.\n");
		return false;
	}	
	
	dev->parent_phone = devman_parent_device_connect(dev->handle,  IPC_FLAG_BLOCKING);
	if (dev->parent_phone <= 0) {
		printf(NAME ": pci_add_device failed to connect to the parent's driver.\n");
		delete_pci_bus_data(bus_data);
		return false;
	}
	
	hw_resource_list_t hw_resources;
	
	if (!get_hw_resources(dev->parent_phone, &hw_resources)) {
		printf(NAME ": pci_add_device failed to get hw resources for the device.\n");
		delete_pci_bus_data(bus_data);
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
		delete_pci_bus_data(bus_data);
		ipc_hangup(dev->parent_phone);
		clean_hw_resource_list(&hw_resources);
		return false;					
	}
	bus_data->conf_data_port = (char *)bus_data->conf_addr_port + 4;
	
	dev->driver_data = bus_data;
	
	// enumerate child devices
	printf(NAME ": scanning the bus\n");
	pci_bus_scan(dev, 0);
	
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