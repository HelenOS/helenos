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
#include <str.h>
#include <ctype.h>
#include <macros.h>

#include <driver.h>
#include <devman.h>
#include <ipc/devman.h>
#include <ipc/dev_iface.h>
#include <ops/hw_res.h>
#include <device/hw_res.h>
#include <ddi.h>
#include <libarch/ddi.h>

#include "pci.h"

#define NAME "pciintel"

#define CONF_ADDR(bus, dev, fn, reg) \
	((1 << 31) | (bus << 16) | (dev << 11) | (fn << 8) | (reg & ~3))

static hw_resource_list_t *pciintel_get_child_resources(function_t *fun)
{
	pci_fun_data_t *fun_data = (pci_fun_data_t *) fun->driver_data;
	
	if (fun_data == NULL)
		return NULL;
	return &fun_data->hw_resources;
}

static bool pciintel_enable_child_interrupt(function_t *fun)
{
	/* TODO */
	
	return false;
}

static hw_res_ops_t pciintel_child_hw_res_ops = {
	&pciintel_get_child_resources,
	&pciintel_enable_child_interrupt
};

static device_ops_t pci_child_ops;

static int pci_add_device(device_t *);

/** The pci bus driver's standard operations. */
static driver_ops_t pci_ops = {
	.add_device = &pci_add_device
};

/** The pci bus driver structure. */
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

static pci_bus_data_t *create_pci_bus_data(void)
{
	pci_bus_data_t *bus_data;
	
	bus_data = (pci_bus_data_t *) malloc(sizeof(pci_bus_data_t));
	if (bus_data != NULL) {
		memset(bus_data, 0, sizeof(pci_bus_data_t));
		fibril_mutex_initialize(&bus_data->conf_mutex);
	}

	return bus_data;
}

static void delete_pci_bus_data(pci_bus_data_t *bus_data)
{
	free(bus_data);
}

static void pci_conf_read(function_t *fun, int reg, uint8_t *buf, size_t len)
{
	assert(fun->dev != NULL);
	
	pci_fun_data_t *fun_data = (pci_fun_data_t *) fun->driver_data;
	pci_bus_data_t *bus_data = (pci_bus_data_t *) fun->dev->driver_data;
	
	fibril_mutex_lock(&bus_data->conf_mutex);
	
	uint32_t conf_addr;
	conf_addr = CONF_ADDR(fun_data->bus, fun_data->dev, fun_data->fn, reg);
	void *addr = bus_data->conf_data_port + (reg & 3);
	
	pio_write_32(bus_data->conf_addr_port, conf_addr);
	
	switch (len) {
	case 1:
		buf[0] = pio_read_8(addr);
		break;
	case 2:
		((uint16_t *) buf)[0] = pio_read_16(addr);
		break;
	case 4:
		((uint32_t *) buf)[0] = pio_read_32(addr);
		break;
	}
	
	fibril_mutex_unlock(&bus_data->conf_mutex);
}

static void pci_conf_write(function_t *fun, int reg, uint8_t *buf, size_t len)
{
	assert(fun->dev != NULL);
	
	pci_fun_data_t *fun_data = (pci_fun_data_t *) fun->driver_data;
	pci_bus_data_t *bus_data = (pci_bus_data_t *) fun->dev->driver_data;
	
	fibril_mutex_lock(&bus_data->conf_mutex);
	
	uint32_t conf_addr;
	conf_addr = CONF_ADDR(fun_data->bus, fun_data->dev, fun_data->fn, reg);
	void *addr = bus_data->conf_data_port + (reg & 3);
	
	pio_write_32(bus_data->conf_addr_port, conf_addr);
	
	switch (len) {
	case 1:
		pio_write_8(addr, buf[0]);
		break;
	case 2:
		pio_write_16(addr, ((uint16_t *) buf)[0]);
		break;
	case 4:
		pio_write_32(addr, ((uint32_t *) buf)[0]);
		break;
	}
	
	fibril_mutex_unlock(&bus_data->conf_mutex);
}

uint8_t pci_conf_read_8(function_t *fun, int reg)
{
	uint8_t res;
	pci_conf_read(fun, reg, &res, 1);
	return res;
}

uint16_t pci_conf_read_16(function_t *fun, int reg)
{
	uint16_t res;
	pci_conf_read(fun, reg, (uint8_t *) &res, 2);
	return res;
}

uint32_t pci_conf_read_32(function_t *fun, int reg)
{
	uint32_t res;
	pci_conf_read(fun, reg, (uint8_t *) &res, 4);
	return res;
}

void pci_conf_write_8(function_t *fun, int reg, uint8_t val)
{
	pci_conf_write(fun, reg, (uint8_t *) &val, 1);
}

void pci_conf_write_16(function_t *fun, int reg, uint16_t val)
{
	pci_conf_write(fun, reg, (uint8_t *) &val, 2);
}

void pci_conf_write_32(function_t *fun, int reg, uint32_t val)
{
	pci_conf_write(fun, reg, (uint8_t *) &val, 4);
}

void create_pci_match_ids(function_t *fun)
{
	pci_fun_data_t *fun_data = (pci_fun_data_t *) fun->driver_data;
	match_id_t *match_id = NULL;
	char *match_id_str;
	
	match_id = create_match_id();
	if (match_id != NULL) {
		asprintf(&match_id_str, "pci/ven=%04x&dev=%04x",
		    fun_data->vendor_id, fun_data->device_id);
		match_id->id = match_id_str;
		match_id->score = 90;
		add_match_id(&fun->match_ids, match_id);
	}

	/* TODO add more ids (with subsys ids, using class id etc.) */
}

void
pci_add_range(function_t *fun, uint64_t range_addr, size_t range_size, bool io)
{
	pci_fun_data_t *fun_data = (pci_fun_data_t *) fun->driver_data;
	hw_resource_list_t *hw_res_list = &fun_data->hw_resources;
	hw_resource_t *hw_resources =  hw_res_list->resources;
	size_t count = hw_res_list->count;
	
	assert(hw_resources != NULL);
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

/** Read the base address register (BAR) of the device and if it contains valid
 * address add it to the devices hw resource list.
 *
 * @param dev	The pci device.
 * @param addr	The address of the BAR in the PCI configuration address space of
 *		the device.
 * @return	The addr the address of the BAR which should be read next.
 */
int pci_read_bar(function_t *fun, int addr)
{	
	/* Value of the BAR */
	uint32_t val, mask;
	/* IO space address */
	bool io;
	/* 64-bit wide address */
	bool addrw64;
	
	/* Size of the io or memory range specified by the BAR */
	size_t range_size;
	/* Beginning of the io or memory range specified by the BAR */
	uint64_t range_addr;
	
	/* Get the value of the BAR. */
	val = pci_conf_read_32(fun, addr);
	
	io = (bool) (val & 1);
	if (io) {
		addrw64 = false;
	} else {
		switch ((val >> 1) & 3) {
		case 0:
			addrw64 = false;
			break;
		case 2:
			addrw64 = true;
			break;
		default:
			/* reserved, go to the next BAR */
			return addr + 4;
		}
	}
	
	/* Get the address mask. */
	pci_conf_write_32(fun, addr, 0xffffffff);
	mask = pci_conf_read_32(fun, addr);
	
	/* Restore the original value. */
	pci_conf_write_32(fun, addr, val);
	val = pci_conf_read_32(fun, addr);
	
	range_size = pci_bar_mask_to_size(mask);
	
	if (addrw64) {
		range_addr = ((uint64_t)pci_conf_read_32(fun, addr + 4) << 32) |
		    (val & 0xfffffff0);
	} else {
		range_addr = (val & 0xfffffff0);
	}
	
	if (range_addr != 0) {
		printf(NAME ": function %s : ", fun->name);
		printf("address = %" PRIx64, range_addr);
		printf(", size = %x\n", (unsigned int) range_size);
	}
	
	pci_add_range(fun, range_addr, range_size, io);
	
	if (addrw64)
		return addr + 8;
	
	return addr + 4;
}

void pci_add_interrupt(function_t *fun, int irq)
{
	pci_fun_data_t *fun_data = (pci_fun_data_t *) fun->driver_data;
	hw_resource_list_t *hw_res_list = &fun_data->hw_resources;
	hw_resource_t *hw_resources = hw_res_list->resources;
	size_t count = hw_res_list->count;
	
	assert(NULL != hw_resources);
	assert(count < PCI_MAX_HW_RES);
	
	hw_resources[count].type = INTERRUPT;
	hw_resources[count].res.interrupt.irq = irq;
	
	hw_res_list->count++;
	
	printf(NAME ": function %s uses irq %x.\n", fun->name, irq);
}

void pci_read_interrupt(function_t *fun)
{
	uint8_t irq = pci_conf_read_8(fun, PCI_BRIDGE_INT_LINE);
	if (irq != 0xff)
		pci_add_interrupt(fun, irq);
}

/** Enumerate (recursively) and register the devices connected to a pci bus.
 *
 * @param dev		The host-to-pci bridge device.
 * @param bus_num	The bus number.
 */
void pci_bus_scan(device_t *dev, int bus_num) 
{
	function_t *fun = create_function();
	pci_fun_data_t *fun_data = create_pci_fun_data();
	fun->driver_data = fun_data;
	
	int child_bus = 0;
	int dnum, fnum;
	bool multi;
	uint8_t header_type;

	/* We need this early, before registering. */
	fun->dev = dev;
	
	for (dnum = 0; dnum < 32; dnum++) {
		multi = true;
		for (fnum = 0; multi && fnum < 8; fnum++) {
			init_pci_fun_data(fun_data, bus_num, dnum, fnum);
			fun_data->vendor_id = pci_conf_read_16(fun,
			    PCI_VENDOR_ID);
			fun_data->device_id = pci_conf_read_16(fun,
			    PCI_DEVICE_ID);
			if (fun_data->vendor_id == 0xffff) {
				/*
				 * The device is not present, go on scanning the
				 * bus.
				 */
				if (fnum == 0)
					break;
				else
					continue;
			}
			
			header_type = pci_conf_read_8(fun, PCI_HEADER_TYPE);
			if (fnum == 0) {
				/* Is the device multifunction? */
				multi = header_type >> 7;
			}
			/* Clear the multifunction bit. */
			header_type = header_type & 0x7F;
			
			create_pci_fun_name(fun);
			
			pci_alloc_resource_list(fun);
			pci_read_bars(fun);
			pci_read_interrupt(fun);
			
			fun->ftype = fun_inner;
			fun->ops = &pci_child_ops;
			
			printf(NAME ": adding new function %s.\n",
			    fun->name);
			
			create_pci_match_ids(fun);
			
			if (register_function(fun, dev) != EOK) {
				pci_clean_resource_list(fun);
				clean_match_ids(&fun->match_ids);
				free((char *) fun->name);
				fun->name = NULL;
				continue;
			}
			
			if (header_type == PCI_HEADER_TYPE_BRIDGE ||
			    header_type == PCI_HEADER_TYPE_CARDBUS) {
				child_bus = pci_conf_read_8(fun,
				    PCI_BRIDGE_SEC_BUS_NUM);
				printf(NAME ": device is pci-to-pci bridge, "
				    "secondary bus number = %d.\n", bus_num);
				if (child_bus > bus_num)
					pci_bus_scan(dev, child_bus);
			}
			
			/* Alloc new aux. fun. structure. */
			fun = create_function();

			/* We need this early, before registering. */
		    	fun->dev = dev;

			fun_data = create_pci_fun_data();
			fun->driver_data = fun_data;
		}
	}
	
	if (fun_data->vendor_id == 0xffff) {
		delete_function(fun);
		/* Free the auxiliary function structure. */
		delete_pci_fun_data(fun_data);
	}
}

static int pci_add_device(device_t *dev)
{
	int rc;

	printf(NAME ": pci_add_device\n");
	
	pci_bus_data_t *bus_data = create_pci_bus_data();
	if (bus_data == NULL) {
		printf(NAME ": pci_add_device allocation failed.\n");
		return ENOMEM;
	}
	
	dev->parent_phone = devman_parent_device_connect(dev->handle,
	    IPC_FLAG_BLOCKING);
	if (dev->parent_phone < 0) {
		printf(NAME ": pci_add_device failed to connect to the "
		    "parent's driver.\n");
		delete_pci_bus_data(bus_data);
		return dev->parent_phone;
	}
	
	hw_resource_list_t hw_resources;
	
	rc = hw_res_get_resource_list(dev->parent_phone, &hw_resources);
	if (rc != EOK) {
		printf(NAME ": pci_add_device failed to get hw resources for "
		    "the device.\n");
		delete_pci_bus_data(bus_data);
		async_hangup(dev->parent_phone);
		return rc;
	}	
	
	printf(NAME ": conf_addr = %" PRIx64 ".\n",
	    hw_resources.resources[0].res.io_range.address);
	
	assert(hw_resources.count > 0);
	assert(hw_resources.resources[0].type == IO_RANGE);
	assert(hw_resources.resources[0].res.io_range.size == 8);
	
	bus_data->conf_io_addr =
	    (uint32_t) hw_resources.resources[0].res.io_range.address;
	
	if (pio_enable((void *)(uintptr_t)bus_data->conf_io_addr, 8,
	    &bus_data->conf_addr_port)) {
		printf(NAME ": failed to enable configuration ports.\n");
		delete_pci_bus_data(bus_data);
		async_hangup(dev->parent_phone);
		hw_res_clean_resource_list(&hw_resources);
		return EADDRNOTAVAIL;
	}
	bus_data->conf_data_port = (char *) bus_data->conf_addr_port + 4;
	
	dev->driver_data = bus_data;
	
	/* Make the bus device more visible. Does not do anything. */
	printf(NAME ": adding a 'ctl' function\n");

	function_t *ctl = create_function();
	ctl->ftype = fun_exposed;
	ctl->name = "ctl";
	register_function(ctl, dev);
	
	/* Enumerate child devices. */
	printf(NAME ": scanning the bus\n");
	pci_bus_scan(dev, 0);
	
	hw_res_clean_resource_list(&hw_resources);
	
	return EOK;
}

static void pciintel_init(void)
{
	pci_child_ops.interfaces[HW_RES_DEV_IFACE] = &pciintel_child_hw_res_ops;
}

pci_fun_data_t *create_pci_fun_data(void)
{
	pci_fun_data_t *res = (pci_fun_data_t *) malloc(sizeof(pci_fun_data_t));
	
	if (res != NULL)
		memset(res, 0, sizeof(pci_fun_data_t));
	return res;
}

void init_pci_fun_data(pci_fun_data_t *fun_data, int bus, int dev, int fn)
{
	fun_data->bus = bus;
	fun_data->dev = dev;
	fun_data->fn = fn;
}

void delete_pci_fun_data(pci_fun_data_t *fun_data)
{
	if (fun_data != NULL) {
		hw_res_clean_resource_list(&fun_data->hw_resources);
		free(fun_data);
	}
}

void create_pci_fun_name(function_t *fun)
{
	pci_fun_data_t *fun_data = (pci_fun_data_t *) fun->driver_data;
	char *name = NULL;
	
	asprintf(&name, "%02x:%02x.%01x", fun_data->bus, fun_data->dev,
	    fun_data->fn);
	fun->name = name;
}

bool pci_alloc_resource_list(function_t *fun)
{
	pci_fun_data_t *fun_data = (pci_fun_data_t *)fun->driver_data;
	
	fun_data->hw_resources.resources =
	    (hw_resource_t *) malloc(PCI_MAX_HW_RES * sizeof(hw_resource_t));
	return fun_data->hw_resources.resources != NULL;
}

void pci_clean_resource_list(function_t *fun)
{
	pci_fun_data_t *fun_data = (pci_fun_data_t *) fun->driver_data;
	
	if (fun_data->hw_resources.resources != NULL) {
		free(fun_data->hw_resources.resources);
		fun_data->hw_resources.resources = NULL;
	}
}

/** Read the base address registers (BARs) of the device and adds the addresses
 * to its hw resource list.
 *
 * @param dev the pci device.
 */
void pci_read_bars(function_t *fun)
{
	/*
	 * Position of the BAR in the PCI configuration address space of the
	 * device.
	 */
	int addr = PCI_BASE_ADDR_0;
	
	while (addr <= PCI_BASE_ADDR_5)
		addr = pci_read_bar(fun, addr);
}

size_t pci_bar_mask_to_size(uint32_t mask)
{
	return ((mask & 0xfffffff0) ^ 0xffffffff) + 1;
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
