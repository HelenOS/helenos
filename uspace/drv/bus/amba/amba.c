/*
 * Copyright (c) 2013 Jakub Klama
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
 * @defgroup amba AMBA bus driver.
 * @brief HelenOS AMBA bus driver.
 * @{
 */
/** @file
 */

#include <assert.h>
#include <byteorder.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <fibril_synch.h>
#include <str.h>
#include <ctype.h>
#include <macros.h>
#include <str_error.h>
#include <ddf/driver.h>
#include <ddf/log.h>
#include <ipc/dev_iface.h>
#include <ipc/irc.h>
#include <ns.h>
#include <ipc/services.h>
#include <sysinfo.h>
#include <ops/hw_res.h>
#include <device/hw_res.h>
#include <ddi.h>
#include "ambapp.h"

#define NAME  "amba"
#define ID_MAX_STR_LEN  32

#define LVL_DEBUG  LVL_ERROR

typedef struct leon_amba_bus {
	/** DDF device node */
	ddf_dev_t *dnode;
	uintptr_t master_area_addr;
	uintptr_t slave_area_addr;
	size_t master_area_size;
	size_t slave_area_size;
	void *master_area;
	void *slave_area;
	fibril_mutex_t area_mutex;
} amba_bus_t;

typedef struct amba_fun_data {
	amba_bus_t *busptr;
	ddf_fun_t *fnode;
	int bus;
	int index;
	uint8_t vendor_id;
	uint32_t device_id;
	int version;
	hw_resource_list_t hw_resources;
	hw_resource_t resources[AMBA_MAX_HW_RES];
} amba_fun_t;

static amba_fun_t *amba_fun_new(amba_bus_t *);
static void amba_fun_set_name(amba_fun_t *);
static void amba_fun_create_match_ids(amba_fun_t *);
static int amba_fun_online(ddf_fun_t *);
static int amba_fun_offline(ddf_fun_t *);
static hw_resource_list_t *amba_get_resources(ddf_fun_t *);
static bool amba_enable_interrupt(ddf_fun_t *);
static void amba_add_bar(amba_fun_t *, uintptr_t, size_t);
static void amba_add_interrupt(amba_fun_t *, int);
static int amba_bus_scan(amba_bus_t *, void *, unsigned int);
static void amba_fake_scan(amba_bus_t *);
static int amba_dev_add(ddf_dev_t *);

static hw_res_ops_t amba_fun_hw_res_ops = {
	.get_resource_list = &amba_get_resources,
	.enable_interrupt = &amba_enable_interrupt
};

static ddf_dev_ops_t amba_fun_ops = {
	.interfaces[HW_RES_DEV_IFACE] = &amba_fun_hw_res_ops
};

static driver_ops_t amba_ops = {
	.dev_add = &amba_dev_add,
	.fun_online = &amba_fun_online,
	.fun_offline = &amba_fun_offline
};

static driver_t amba_driver = {
	.name = NAME,
	.driver_ops = &amba_ops
};

static amba_fun_t *amba_fun_new(amba_bus_t *bus)
{
	ddf_msg(LVL_DEBUG, "amba_fun_new(): bus=%p, bus->dnode=%p", bus,
	    bus->dnode);
	
	ddf_fun_t *fnode = ddf_fun_create(bus->dnode, fun_inner, NULL);
	if (fnode == NULL)
		return NULL;
	
	ddf_msg(LVL_DEBUG, "amba_fun_new(): created");
	
	amba_fun_t *fun = ddf_fun_data_alloc(fnode, sizeof(amba_fun_t));
	if (fun == NULL)
		return NULL;
	
	ddf_msg(LVL_DEBUG, "amba_fun_new(): allocated data");
	
	fun->busptr = bus;
	fun->fnode = fnode;
	return fun;
}

static void amba_fun_set_name(amba_fun_t *fun)
{
	char *name = NULL;
	
	asprintf(&name, "%02x:%02x", fun->bus, fun->index);
	ddf_fun_set_name(fun->fnode, name);
}

static void amba_fun_create_match_ids(amba_fun_t *fun)
{
	/* Vendor ID & Device ID */
	char match_id_str[ID_MAX_STR_LEN];
	int rc = snprintf(match_id_str, ID_MAX_STR_LEN, "amba/ven=%02x&dev=%08x",
	    fun->vendor_id, fun->device_id);
	if (rc < 0) {
		ddf_msg(LVL_ERROR, "Failed creating match ID str: %s",
		    str_error(rc));
	}
	
	rc = ddf_fun_add_match_id(fun->fnode, match_id_str, 90);
}

static int amba_fun_online(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "amba_fun_online()");
	return ddf_fun_online(fun);

}

static int amba_fun_offline(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "amba_fun_offline()");
	return ddf_fun_offline(fun);
}

static hw_resource_list_t *amba_get_resources(ddf_fun_t *fnode)
{
	amba_fun_t *fun = ddf_fun_data_get(fnode);
	
	if (fun == NULL)
		return NULL;
	
	return &fun->hw_resources;
}

static bool amba_enable_interrupt(ddf_fun_t *fnode)
{
	return true;
}

static void amba_alloc_resource_list(amba_fun_t *fun)
{
	fun->hw_resources.resources = fun->resources;
}

static void amba_add_bar(amba_fun_t *fun, uintptr_t addr, size_t size)
{
	hw_resource_list_t *hw_res_list = &fun->hw_resources;
	hw_resource_t *hw_resources =  hw_res_list->resources;
	size_t count = hw_res_list->count;
	
	assert(hw_resources != NULL);
	assert(count < AMBA_MAX_HW_RES);
	
	hw_resources[count].type = MEM_RANGE;
	hw_resources[count].res.mem_range.address = addr;
	hw_resources[count].res.mem_range.size = size;
	hw_resources[count].res.mem_range.endianness = BIG_ENDIAN;
	
	hw_res_list->count++;
}

static void amba_add_interrupt(amba_fun_t *fun, int irq)
{
	hw_resource_list_t *hw_res_list = &fun->hw_resources;
	hw_resource_t *hw_resources = hw_res_list->resources;
	size_t count = hw_res_list->count;
	
	assert(NULL != hw_resources);
	assert(count < AMBA_MAX_HW_RES);
	
	hw_resources[count].type = INTERRUPT;
	hw_resources[count].res.interrupt.irq = irq;
	
	hw_res_list->count++;
	
	ddf_msg(LVL_NOTE, "Function %s uses irq %x.", ddf_fun_get_name(fun->fnode), irq);
}

static int amba_bus_scan(amba_bus_t *bus, void *area, unsigned int max_entries)
{
	ddf_msg(LVL_DEBUG, "amba_bus_scan(): area=%p, max_entries=%u", area, max_entries);
	
	ambapp_entry_t *devices = (ambapp_entry_t *) area;
	int found = 0;
	
	for (unsigned int i = 0; i < max_entries; i++) {
		ambapp_entry_t *entry = &devices[i];
		if (entry->vendor_id == 0xff)
			continue;
		
		amba_fun_t *fun = amba_fun_new(bus);
		fun->bus = 0;
		fun->index = i;
		fun->vendor_id = entry->vendor_id;
		fun->device_id = entry->device_id;
		fun->version = entry->version;
		
		for (unsigned int bnum = 0; bnum < 4; bnum++) {
			ambapp_bar_t *bar = &entry->bar[bnum];
			amba_add_bar(fun, bar->addr << 20, bar->mask);
		}
		
		if (entry->irq != -1)
			amba_add_interrupt(fun, entry->irq);
		
		ddf_fun_set_ops(fun->fnode, &amba_fun_ops);
		ddf_fun_bind(fun->fnode);
	}
	
	return found;
}

static void amba_fake_scan(amba_bus_t *bus)
{
	ddf_msg(LVL_DEBUG, "amba_fake_scan()");
	
	/* UART */
	amba_fun_t *fun = amba_fun_new(bus);
	fun->bus = 0;
	fun->index = 0;
	fun->vendor_id = GAISLER;
	fun->device_id = GAISLER_APBUART;
	fun->version = 1;
	amba_alloc_resource_list(fun);
	amba_add_bar(fun, 0x80000100, 0x100);
	amba_add_interrupt(fun, 3);
	amba_fun_set_name(fun);
	amba_fun_create_match_ids(fun);
	ddf_fun_set_ops(fun->fnode, &amba_fun_ops);
	ddf_fun_bind(fun->fnode);
	
	ddf_msg(LVL_DEBUG, "added uart");
	
	/* IRQMP */
	fun = amba_fun_new(bus);
	fun->bus = 0;
	fun->index = 1;
	fun->vendor_id = GAISLER;
	fun->device_id = GAISLER_IRQMP;
	fun->version = 1;
	amba_alloc_resource_list(fun);
	amba_add_bar(fun, 0x80000200, 0x100);
	amba_fun_set_name(fun);
	amba_fun_create_match_ids(fun);
	ddf_fun_set_ops(fun->fnode, &amba_fun_ops);
	ddf_fun_bind(fun->fnode);
	
	ddf_msg(LVL_DEBUG, "added irqmp");
	
	/* GPTIMER */
	fun = amba_fun_new(bus);
	fun->bus = 0;
	fun->index = 2;
	fun->vendor_id = GAISLER;
	fun->device_id = GAISLER_GPTIMER;
	fun->version = 1;
	amba_alloc_resource_list(fun);
	amba_add_bar(fun, 0x80000300, 0x100);
	amba_add_interrupt(fun, 8);
	amba_fun_set_name(fun);
	amba_fun_create_match_ids(fun);
	ddf_fun_set_ops(fun->fnode, &amba_fun_ops);
	ddf_fun_bind(fun->fnode);
	
	ddf_msg(LVL_DEBUG, "added timer");
}

static int amba_dev_add(ddf_dev_t *dnode)
{
	int rc = 0;
	int found = 0;
	bool got_res = false;
	
	amba_bus_t *bus = ddf_dev_data_alloc(dnode, sizeof(amba_bus_t));
	if (bus == NULL) {
		ddf_msg(LVL_ERROR, "amba_dev_add: allocation failed.");
		rc = ENOMEM;
		goto fail;
	}
	
	bus->dnode = dnode;
	async_sess_t *sess = ddf_dev_parent_sess_create(dnode, EXCHANGE_SERIALIZE);
	if (sess == NULL) {
		ddf_msg(LVL_ERROR, "amba_dev_add failed to connect to the "
		    "parent driver.");
		rc = ENOENT;
		goto fail;
	}
	
	hw_resource_list_t hw_resources;
	rc = hw_res_get_resource_list(sess, &hw_resources);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "amba_dev_add failed to get hw resources "
		    "for the device.");
		goto fail;
	}
	
	got_res = true;
	assert(hw_resources.count > 1);
	
	bus->master_area_addr = hw_resources.resources[0].res.mem_range.address;
	bus->master_area_size = hw_resources.resources[0].res.mem_range.size;
	bus->slave_area_addr = hw_resources.resources[1].res.mem_range.address;
	bus->slave_area_size = hw_resources.resources[1].res.mem_range.size;
	
	ddf_msg(LVL_DEBUG, "AMBA master area: 0x%08x", bus->master_area_addr);
	ddf_msg(LVL_DEBUG, "AMBA slave area: 0x%08x", bus->slave_area_addr);
	
	if (pio_enable((void *)bus->master_area_addr, bus->master_area_size, &bus->master_area)) {
		ddf_msg(LVL_ERROR, "Failed to enable master area.");
		rc = EADDRNOTAVAIL;
		goto fail;
	}
	
	if (pio_enable((void *)bus->slave_area_addr, bus->slave_area_size, &bus->slave_area)) {
		ddf_msg(LVL_ERROR, "Failed to enable slave area.");
		rc = EADDRNOTAVAIL;
		goto fail;
	}
	
	/*
	 * If nothing is found, we are probably running inside QEMU
	 * and need to fake AMBA P&P entries.
	 */
	if (found == 0)
		amba_fake_scan(bus);
	
	ddf_msg(LVL_DEBUG, "done");
	
	return EOK;
	
fail:
	if (got_res)
		hw_res_clean_resource_list(&hw_resources);
	
	return rc;
}

int main(int argc, char *argv[])
{
	printf("%s: HelenOS LEON3 AMBA bus driver\n", NAME);
	ddf_log_init(NAME);
	return ddf_driver_main(&amba_driver);
}
