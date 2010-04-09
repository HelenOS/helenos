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

#define NAME "pciintel"

#define CONF_ADDR(bus, dev, fn, reg)   ((1 << 31) | (bus << 16) | (dev << 11) | (fn << 8) | (reg & ~3))


static hw_resource_list_t * pciintel_get_child_resources(device_t *dev)
{
	pci_dev_data_t *dev_data = (pci_dev_data_t *)dev->driver_data;
	if (NULL == dev_data) {
		return NULL;
	}
	return &dev_data->hw_resources;
}

static bool pciintel_enable_child_interrupt(device_t *dev) 
{
	// TODO
	
	return false;
}

static resource_iface_t pciintel_child_res_iface = {
	&pciintel_get_child_resources,
	&pciintel_enable_child_interrupt	
};

static device_class_t pci_child_class;


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
	uint32_t conf_io_addr;
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

static void pci_conf_write(device_t *dev, int reg, uint8_t *buf, size_t len)
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
			pio_write_8(addr, buf[0]);
			break;
		case 2:
			pio_write_16(addr, ((uint16_t *)buf)[0]);
			break;
		case 4:
			pio_write_32(addr, ((uint32_t *)buf)[0]);
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

void pci_conf_write_8(device_t *dev, int reg, uint8_t val) 
{
	pci_conf_write(dev, reg, (uint8_t *)&val, 1);	
}

void pci_conf_write_16(device_t *dev, int reg, uint16_t val) 
{
	pci_conf_write(dev, reg, (uint8_t *)&val, 2);	
}

void pci_conf_write_32(device_t *dev, int reg, uint32_t val) 
{
	pci_conf_write(dev, reg, (uint8_t *)&val, 4);	
}


void create_pci_match_ids(device_t *dev)
{
	pci_dev_data_t *dev_data = (pci_dev_data_t *)dev->driver_data;
	match_id_t *match_id = NULL;	
	match_id = create_match_id();
	if (NULL != match_id) {
		asprintf(&match_id->id, "pci/ven=%04x&dev=%04x", dev_data->vendor_id, dev_data->device_id);
		match_id->score = 90;
		add_match_id(&dev->match_ids, match_id);
	}	
	// TODO add more ids (with subsys ids, using class id etc.)
}

void pci_add_range(device_t *dev, uint64_t range_addr, size_t range_size, bool io)
{
	pci_dev_data_t *dev_data = (pci_dev_data_t *)dev->driver_data;
	hw_resource_list_t *hw_res_list = &dev_data->hw_resources;
	hw_resource_t *hw_resources =  hw_res_list->resources;
	size_t count = hw_res_list->count;	
	
	assert(NULL != hw_resources);
	assert(count < PCI_MAX_HW_RES);
	
	if (io) {
		hw_resources[count].type = IO_RANGE;
		hw_resources[count].res.io_range.address = range_addr;
		hw_resources[count].res.io_range.size = range_size;	
		hw_resources[count].res.io_range.endianness = LITTLE_ENDIAN;	
	} else {
		hw_resources[count].type = MEM_RANGE;
		hw_resources[count].res.mem_range.address = range_addr;
		hw_resources[count].res.mem_range.size = range_size;	
		hw_resources[count].res.mem_range.endianness = LITTLE_ENDIAN;
	}
	
	hw_res_list->count++;	
}


/** Read the base address register (BAR) of the device 
 *  and if it contains valid address add it to the devices hw resource list.
 * 
 * @param dev the pci device.
 * @param addr the address of the BAR in the PCI configuration address space of the device.
 * 
 * @return the addr the address of the BAR which should be read next.
 */
int pci_read_bar(device_t *dev, int addr) 
{	
	// value of the BAR
	uint32_t val, mask;
	// IO space address
	bool io;
	// 64-bit wide address
	bool w64;
	
	// size of the io or memory range specified by the BAR
	size_t range_size;
	// beginning of the io or memory range specified by the BAR
	uint64_t range_addr;
	
	// get the value of the BAR
	val = pci_conf_read_32(dev, addr);
	
	io = (bool)(val & 1);
	if (io) {
		w64 = false;
	} else {
		switch ((val >> 1) & 3) {
		case 0:
			w64 = false;
			break;
		case 2:
			w64 = true;
			break;
		default:
			// reserved, go to the next BAR
			return addr + 4;							
		}
	}
	
	// get the address mask
	pci_conf_write_32(dev, addr, 0xffffffff);
	mask = pci_conf_read_32(dev, addr);	
	
	// restore the original value
	pci_conf_write_32(dev, addr, val);
	val = pci_conf_read_32(dev, addr);	
	
	range_size = pci_bar_mask_to_size(mask);
	
	if (w64) {
		range_addr = ((uint64_t)pci_conf_read_32(dev, addr + 4) << 32) | (val & 0xfffffff0);	
	} else {
		range_addr = (val & 0xfffffff0);
	}	
	if (0 != range_addr) {
		printf(NAME ": device %s : ", dev->name);
		printf("address = %x", range_addr);		
		printf(", size = %x\n", range_size);
	}
	
	pci_add_range(dev, range_addr, range_size, io);
	
	if (w64) {
		return addr + 8;
	}
	return addr + 4;	
}

void pci_add_interrupt(device_t *dev, int irq)
{
	pci_dev_data_t *dev_data = (pci_dev_data_t *)dev->driver_data;
	hw_resource_list_t *hw_res_list = &dev_data->hw_resources;
	hw_resource_t *hw_resources =  hw_res_list->resources;
	size_t count = hw_res_list->count;	
	
	assert(NULL != hw_resources);
	assert(count < PCI_MAX_HW_RES);
	
	hw_resources[count].type = INTERRUPT;
	hw_resources[count].res.interrupt.irq = irq;
	
	hw_res_list->count++;		
	
	
	printf(NAME ": device %s uses irq %x.\n", dev->name, irq);
}

void pci_read_interrupt(device_t *dev)
{
	uint8_t irq = pci_conf_read_8(dev, PCI_BRIDGE_INT_LINE);
	if (0xff != irq) {
		pci_add_interrupt(dev, irq);
	}	
}

/** Enumerate (recursively) and register the devices connected to a pci bus.
 * 
 * @param parent the host-to-pci bridge device.
 * @param bus_num the bus number. 
 */
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
			if (dev_data->vendor_id == 0xffff) { // device is not present, go on scanning the bus
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
			
			create_pci_dev_name(dev);
			
			pci_alloc_resource_list(dev);
			pci_read_bars(dev);
			pci_read_interrupt(dev);
			
			dev->class = &pci_child_class;			
			
			printf(NAME ": adding new child device %s.\n", dev->name);
			
			create_pci_match_ids(dev);
			
			if (!child_device_register(dev, parent)) {				
				pci_clean_resource_list(dev);				
				clean_match_ids(&dev->match_ids);
				free((char *)dev->name);
				dev->name = NULL;
				continue;
			}
			
			//printf(NAME ": new device %s was successfully registered by device manager.\n", dev->name);
			
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
	
	if (dev_data->vendor_id == 0xffff) {
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
	
	printf(NAME ": conf_addr = %x.\n", hw_resources.resources[0].res.io_range.address);	
	
	assert(hw_resources.count > 0);
	assert(hw_resources.resources[0].type == IO_RANGE);
	assert(hw_resources.resources[0].res.io_range.size == 8);
	
	bus_data->conf_io_addr = (uint32_t)hw_resources.resources[0].res.io_range.address;
	
	if (pio_enable((void *)bus_data->conf_io_addr, 8, &bus_data->conf_addr_port)) {
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

static void pciintel_init() 
{
	pci_child_class.id = 0; // TODO
	pci_child_class.interfaces[HW_RES_DEV_IFACE] = &pciintel_child_res_iface;
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS pci bus driver (intel method 1).\n");
	pciintel_init();
	return driver_main(&pci_driver);
}

/**
 * @}
 */