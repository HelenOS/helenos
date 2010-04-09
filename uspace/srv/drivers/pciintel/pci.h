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

/** @addtogroup pciintel
 * @{
 */
/** @file
 */
 
#ifndef PCI_H
#define PCI_H


#include <stdlib.h>
#include <driver.h>
#include <malloc.h>

#include "pci_regs.h"

#define PCI_MAX_HW_RES 8

typedef struct pci_dev_data {
	int bus;
	int dev;
	int fn;
	int vendor_id;
	int device_id;
	hw_resource_list_t hw_resources;
} pci_dev_data_t;

void create_pci_match_ids(device_t *dev);

uint8_t pci_conf_read_8(device_t *dev, int reg);
uint16_t pci_conf_read_16(device_t *dev, int reg);
uint32_t pci_conf_read_32(device_t *dev, int reg);
void pci_conf_write_8(device_t *dev, int reg, uint8_t val);
void pci_conf_write_16(device_t *dev, int reg, uint16_t val);
void pci_conf_write_32(device_t *dev, int reg, uint32_t val);

void pci_add_range(device_t *dev, uint64_t range_addr, size_t range_size, bool io);
int pci_read_bar(device_t *dev, int addr);
void pci_read_interrupt(device_t *dev);
void pci_add_interrupt(device_t *dev, int irq);

void pci_bus_scan(device_t *parent, int bus_num);


static inline pci_dev_data_t *create_pci_dev_data() 
{
	pci_dev_data_t *res = (pci_dev_data_t *)malloc(sizeof(pci_dev_data_t));
	if (NULL != res) {
		memset(res, 0, sizeof(pci_dev_data_t));
	}
	return res;	
}

static inline void init_pci_dev_data(pci_dev_data_t *d, int bus, int dev, int fn) 
{
	d->bus = bus;
	d->dev = dev;
	d->fn = fn;	
}

static inline void delete_pci_dev_data(pci_dev_data_t *d) 
{
	if (NULL != d) {
		clean_hw_resource_list(&d->hw_resources);
		free(d);	
	}
}

static inline void create_pci_dev_name(device_t *dev)
{
	pci_dev_data_t *dev_data = (pci_dev_data_t *)dev->driver_data;
	char *name = NULL;
	asprintf(&name, "%02x:%02x.%01x", dev_data->bus, dev_data->dev, dev_data->fn);
	dev->name = name;
}

static inline bool pci_alloc_resource_list(device_t *dev)
{
	pci_dev_data_t *dev_data = (pci_dev_data_t *)dev->driver_data;
	dev_data->hw_resources.resources = (hw_resource_t *)malloc(PCI_MAX_HW_RES * sizeof(hw_resource_t));
	return dev_data->hw_resources.resources != NULL;	
}

static inline void pci_clean_resource_list(device_t *dev)
{
	pci_dev_data_t *dev_data = (pci_dev_data_t *)dev->driver_data;
	if (NULL != dev_data->hw_resources.resources) {
		free(dev_data->hw_resources.resources);
		dev_data->hw_resources.resources = NULL;
	}
}

/** Read the base address registers (BARs) of the device 
 *  and adds the addresses to its hw resource list.
 * 
 * @param dev the pci device.
 */
static inline  void pci_read_bars(device_t *dev)
{
	// position of the BAR in the PCI configuration address space of the device
	int addr = PCI_BASE_ADDR_0;
	
	while (addr <= PCI_BASE_ADDR_5) {
		addr = pci_read_bar(dev, addr);	
	}	
}

static inline size_t pci_bar_mask_to_size(uint32_t mask)
{
	return ((mask & 0xfffffff0) ^ 0xffffffff) + 1;
}


#endif


/**
 * @}
 */