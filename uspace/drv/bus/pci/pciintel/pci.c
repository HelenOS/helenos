/*
 * Copyright (c) 2010 Lenka Trochtova
 * Copyright (c) 2011 Jiri Svoboda
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
#include <irc.h>
#include <ops/hw_res.h>
#include <device/hw_res.h>
#include <ops/pio_window.h>
#include <device/pio_window.h>
#include <ddi.h>
#include <pci_dev_iface.h>

#include "pci.h"

#define NAME "pciintel"

#define CONF_ADDR_ENABLE	(1 << 31)
#define CONF_ADDR(bus, dev, fn, reg) \
	((bus << 16) | (dev << 11) | (fn << 8) | (reg & ~3))

/** Obtain PCI function soft-state from DDF function node */
static pci_fun_t *pci_fun(ddf_fun_t *fnode)
{
	return ddf_fun_data_get(fnode);
}

/** Obtain PCI bus soft-state from DDF device node */
#if 0
static pci_bus_t *pci_bus(ddf_dev_t *dnode)
{
	return ddf_dev_data_get(dnode);
}
#endif

/** Obtain PCI bus soft-state from function soft-state */
static pci_bus_t *pci_bus_from_fun(pci_fun_t *fun)
{
	return fun->busptr;
}

/** Max is 47, align to something nice. */
#define ID_MAX_STR_LEN 50

static hw_resource_list_t *pciintel_get_resources(ddf_fun_t *fnode)
{
	pci_fun_t *fun = pci_fun(fnode);

	if (fun == NULL)
		return NULL;
	return &fun->hw_resources;
}

static bool pciintel_fun_owns_interrupt(pci_fun_t *fun, int irq)
{
	size_t i;
	hw_resource_list_t *res = &fun->hw_resources;

	for (i = 0; i < res->count; i++) {
		if (res->resources[i].type == INTERRUPT &&
		    res->resources[i].res.interrupt.irq == irq) {
			return true;
		}
	}

	return false;
}

static errno_t pciintel_enable_interrupt(ddf_fun_t *fnode, int irq)
{
	pci_fun_t *fun = pci_fun(fnode);

	if (!pciintel_fun_owns_interrupt(fun, irq))
		return EINVAL;

	return irc_enable_interrupt(irq);
}

static errno_t pciintel_disable_interrupt(ddf_fun_t *fnode, int irq)
{
	pci_fun_t *fun = pci_fun(fnode);

	if (!pciintel_fun_owns_interrupt(fun, irq))
		return EINVAL;

	return irc_disable_interrupt(irq);
}

static errno_t pciintel_clear_interrupt(ddf_fun_t *fnode, int irq)
{
	pci_fun_t *fun = pci_fun(fnode);

	if (!pciintel_fun_owns_interrupt(fun, irq))
		return EINVAL;

	return irc_clear_interrupt(irq);
}

static pio_window_t *pciintel_get_pio_window(ddf_fun_t *fnode)
{
	pci_fun_t *fun = pci_fun(fnode);

	if (fun == NULL)
		return NULL;
	return &fun->pio_window;
}


static errno_t config_space_write_32(ddf_fun_t *fun, uint32_t address,
    uint32_t data)
{
	if (address > 252)
		return EINVAL;
	pci_conf_write_32(pci_fun(fun), address, data);
	return EOK;
}

static errno_t config_space_write_16(
    ddf_fun_t *fun, uint32_t address, uint16_t data)
{
	if (address > 254)
		return EINVAL;
	pci_conf_write_16(pci_fun(fun), address, data);
	return EOK;
}

static errno_t config_space_write_8(
    ddf_fun_t *fun, uint32_t address, uint8_t data)
{
	if (address > 255)
		return EINVAL;
	pci_conf_write_8(pci_fun(fun), address, data);
	return EOK;
}

static errno_t config_space_read_32(
    ddf_fun_t *fun, uint32_t address, uint32_t *data)
{
	if (address > 252)
		return EINVAL;
	*data = pci_conf_read_32(pci_fun(fun), address);
	return EOK;
}

static errno_t config_space_read_16(
    ddf_fun_t *fun, uint32_t address, uint16_t *data)
{
	if (address > 254)
		return EINVAL;
	*data = pci_conf_read_16(pci_fun(fun), address);
	return EOK;
}

static errno_t config_space_read_8(
    ddf_fun_t *fun, uint32_t address, uint8_t *data)
{
	if (address > 255)
		return EINVAL;
	*data = pci_conf_read_8(pci_fun(fun), address);
	return EOK;
}

static hw_res_ops_t pciintel_hw_res_ops = {
	.get_resource_list = &pciintel_get_resources,
	.enable_interrupt = &pciintel_enable_interrupt,
	.disable_interrupt = &pciintel_disable_interrupt,
	.clear_interrupt = &pciintel_clear_interrupt,
};

static pio_window_ops_t pciintel_pio_window_ops = {
	.get_pio_window = &pciintel_get_pio_window
};

static pci_dev_iface_t pci_dev_ops = {
	.config_space_read_8 = &config_space_read_8,
	.config_space_read_16 = &config_space_read_16,
	.config_space_read_32 = &config_space_read_32,
	.config_space_write_8 = &config_space_write_8,
	.config_space_write_16 = &config_space_write_16,
	.config_space_write_32 = &config_space_write_32
};

static ddf_dev_ops_t pci_fun_ops = {
	.interfaces[HW_RES_DEV_IFACE] = &pciintel_hw_res_ops,
	.interfaces[PIO_WINDOW_DEV_IFACE] = &pciintel_pio_window_ops,
	.interfaces[PCI_DEV_IFACE] = &pci_dev_ops
};

static errno_t pci_dev_add(ddf_dev_t *);
static errno_t pci_fun_online(ddf_fun_t *);
static errno_t pci_fun_offline(ddf_fun_t *);

/** PCI bus driver standard operations */
static driver_ops_t pci_ops = {
	.dev_add = &pci_dev_add,
	.fun_online = &pci_fun_online,
	.fun_offline = &pci_fun_offline,
};

/** PCI bus driver structure */
static driver_t pci_driver = {
	.name = NAME,
	.driver_ops = &pci_ops
};

static void pci_conf_read(pci_fun_t *fun, int reg, uint8_t *buf, size_t len)
{
	const uint32_t conf_addr = CONF_ADDR(fun->bus, fun->dev, fun->fn, reg);
	pci_bus_t *bus = pci_bus_from_fun(fun);
	uint32_t val;

	fibril_mutex_lock(&bus->conf_mutex);

	if (bus->conf_addr_reg) {
		pio_write_32(bus->conf_addr_reg,
		    host2uint32_t_le(CONF_ADDR_ENABLE | conf_addr));
		/*
		 * Always read full 32-bits from the PCI conf_data_port
		 * register and get the desired portion of it afterwards. Some
		 * architectures do not support shorter PIO reads offset from
		 * this register.
	 	 */
		val = uint32_t_le2host(pio_read_32(bus->conf_data_reg));
	} else {
		val = uint32_t_le2host(pio_read_32(
		    &bus->conf_space[conf_addr / sizeof(ioport32_t)]));
	}

	switch (len) {
	case 1:
		*buf = (uint8_t) (val >> ((reg & 3) * 8));
		break;
	case 2:
		*((uint16_t *) buf) = (uint16_t) (val >> ((reg & 3)) * 8);
		break;
	case 4:
		*((uint32_t *) buf) = (uint32_t) val;
		break;
	}

	fibril_mutex_unlock(&bus->conf_mutex);
}

static void pci_conf_write(pci_fun_t *fun, int reg, uint8_t *buf, size_t len)
{
	const uint32_t conf_addr = CONF_ADDR(fun->bus, fun->dev, fun->fn, reg);
	pci_bus_t *bus = pci_bus_from_fun(fun);
	uint32_t val = 0;

	fibril_mutex_lock(&bus->conf_mutex);

	/*
	 * Prepare to write full 32-bits to the PCI conf_data_port register.
	 * Some architectures do not support shorter PIO writes offset from this
	 * register.
 	 */

	if (len < 4) {
		/*
 		 * We have fewer than full 32-bits, so we need to read the
 		 * missing bits first.
 		 */
		if (bus->conf_addr_reg) {
			pio_write_32(bus->conf_addr_reg,
			    host2uint32_t_le(CONF_ADDR_ENABLE | conf_addr));
			val = uint32_t_le2host(pio_read_32(bus->conf_data_reg));
		} else {
			val = uint32_t_le2host(pio_read_32(
			    &bus->conf_space[conf_addr / sizeof(ioport32_t)]));
		}
	}

	switch (len) {
	case 1:
		val &= ~(0xffU << ((reg & 3) * 8));
		val |= *buf << ((reg & 3) * 8);
		break;
	case 2:
		val &= ~(0xffffU << ((reg & 3) * 8));
		val |= *((uint16_t *) buf) << ((reg & 3) * 8);
		break;
	case 4:
		val = *((uint32_t *) buf);
		break;
	}

	if (bus->conf_addr_reg) {
		pio_write_32(bus->conf_addr_reg,
		    host2uint32_t_le(CONF_ADDR_ENABLE | conf_addr));
		pio_write_32(bus->conf_data_reg, host2uint32_t_le(val));
	} else {
		pio_write_32(&bus->conf_space[conf_addr / sizeof(ioport32_t)],
		    host2uint32_t_le(val));
	}

	fibril_mutex_unlock(&bus->conf_mutex);
}

uint8_t pci_conf_read_8(pci_fun_t *fun, int reg)
{
	uint8_t res;
	pci_conf_read(fun, reg, &res, 1);
	return res;
}

uint16_t pci_conf_read_16(pci_fun_t *fun, int reg)
{
	uint16_t res;
	pci_conf_read(fun, reg, (uint8_t *) &res, 2);
	return res;
}

uint32_t pci_conf_read_32(pci_fun_t *fun, int reg)
{
	uint32_t res;
	pci_conf_read(fun, reg, (uint8_t *) &res, 4);
	return res;
}

void pci_conf_write_8(pci_fun_t *fun, int reg, uint8_t val)
{
	pci_conf_write(fun, reg, (uint8_t *) &val, 1);
}

void pci_conf_write_16(pci_fun_t *fun, int reg, uint16_t val)
{
	pci_conf_write(fun, reg, (uint8_t *) &val, 2);
}

void pci_conf_write_32(pci_fun_t *fun, int reg, uint32_t val)
{
	pci_conf_write(fun, reg, (uint8_t *) &val, 4);
}

void pci_fun_create_match_ids(pci_fun_t *fun)
{
	errno_t rc;
	int ret;
	char match_id_str[ID_MAX_STR_LEN];

	/* Vendor ID & Device ID, length(incl \0) 22 */
	ret = snprintf(match_id_str, ID_MAX_STR_LEN, "pci/ven=%04"
	    PRIx16 "&dev=%04" PRIx16, fun->vendor_id, fun->device_id);
	if (ret < 0) {
		ddf_msg(LVL_ERROR, "Failed creating match ID str");
	}

	rc = ddf_fun_add_match_id(fun->fnode, match_id_str, 90);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed adding match ID: %s", str_error(rc));
	}

	/* Class, subclass, prog IF, revision, length(incl \0) 47 */
	ret = snprintf(match_id_str, ID_MAX_STR_LEN,
	    "pci/class=%02x&subclass=%02x&progif=%02x&revision=%02x",
	    fun->class_code, fun->subclass_code, fun->prog_if, fun->revision);
	if (ret < 0) {
		ddf_msg(LVL_ERROR, "Failed creating match ID str");
	}

	rc = ddf_fun_add_match_id(fun->fnode, match_id_str, 70);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed adding match ID: %s", str_error(rc));
	}

	/* Class, subclass, prog IF, length(incl \0) 35 */
	ret = snprintf(match_id_str, ID_MAX_STR_LEN,
	    "pci/class=%02x&subclass=%02x&progif=%02x",
	    fun->class_code, fun->subclass_code, fun->prog_if);
	if (ret < 0) {
		ddf_msg(LVL_ERROR, "Failed creating match ID str");
	}

	rc = ddf_fun_add_match_id(fun->fnode, match_id_str, 60);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed adding match ID: %s", str_error(rc));
	}

	/* Class, subclass, length(incl \0) 25 */
	ret = snprintf(match_id_str, ID_MAX_STR_LEN,
	    "pci/class=%02x&subclass=%02x",
	    fun->class_code, fun->subclass_code);
	if (ret < 0) {
		ddf_msg(LVL_ERROR, "Failed creating match ID str");
	}

	rc = ddf_fun_add_match_id(fun->fnode, match_id_str, 50);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed adding match ID: %s", str_error(rc));
	}

	/* Class, length(incl \0) 13 */
	ret = snprintf(match_id_str, ID_MAX_STR_LEN, "pci/class=%02x",
	    fun->class_code);
	if (ret < 0) {
		ddf_msg(LVL_ERROR, "Failed creating match ID str");
	}

	rc = ddf_fun_add_match_id(fun->fnode, match_id_str, 40);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed adding match ID: %s", str_error(rc));
	}

	/* TODO add subsys ids, but those exist only in header type 0 */
}

void pci_add_range(pci_fun_t *fun, uint64_t range_addr, size_t range_size,
    bool io)
{
	hw_resource_list_t *hw_res_list = &fun->hw_resources;
	hw_resource_t *hw_resources =  hw_res_list->resources;
	size_t count = hw_res_list->count;

	assert(hw_resources != NULL);
	assert(count < PCI_MAX_HW_RES);

	if (io) {
		hw_resources[count].type = IO_RANGE;
		hw_resources[count].res.io_range.address = range_addr;
		hw_resources[count].res.io_range.size = range_size;
		hw_resources[count].res.io_range.relative = true;
		hw_resources[count].res.io_range.endianness = LITTLE_ENDIAN;
	} else {
		hw_resources[count].type = MEM_RANGE;
		hw_resources[count].res.mem_range.address = range_addr;
		hw_resources[count].res.mem_range.size = range_size;
		hw_resources[count].res.mem_range.relative = false;
		hw_resources[count].res.mem_range.endianness = LITTLE_ENDIAN;
	}

	hw_res_list->count++;
}

/** Read the base address register (BAR) of the device and if it contains valid
 * address add it to the devices hw resource list.
 *
 * @param fun	PCI function
 * @param addr	The address of the BAR in the PCI configuration address space of
 *		the device
 * @return	The addr the address of the BAR which should be read next
 */
int pci_read_bar(pci_fun_t *fun, int addr)
{
	/* Value of the BAR */
	uint32_t val;
	uint32_t bar;
	uint32_t mask;

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

#define IO_MASK  (~0x3)
#define MEM_MASK (~0xf)

	io = (val & 1) != 0;
	if (io) {
		addrw64 = false;
		mask = IO_MASK;
	} else {
		mask = MEM_MASK;
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
	bar = pci_conf_read_32(fun, addr);

	/*
 	 * Unimplemented BARs read back as all 0's.
 	 */
	if (!bar)
		return addr + (addrw64 ? 8 : 4);

	mask &= bar;

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
		ddf_msg(LVL_DEBUG, "Function %s : address = %" PRIx64
		    ", size = %x", ddf_fun_get_name(fun->fnode), range_addr,
		    (unsigned int) range_size);
	}

	pci_add_range(fun, range_addr, range_size, io);

	if (addrw64)
		return addr + 8;

	return addr + 4;
}

void pci_add_interrupt(pci_fun_t *fun, int irq)
{
	hw_resource_list_t *hw_res_list = &fun->hw_resources;
	hw_resource_t *hw_resources = hw_res_list->resources;
	size_t count = hw_res_list->count;

	assert(NULL != hw_resources);
	assert(count < PCI_MAX_HW_RES);

	hw_resources[count].type = INTERRUPT;
	hw_resources[count].res.interrupt.irq = irq;

	hw_res_list->count++;

	ddf_msg(LVL_NOTE, "Function %s uses irq %x.", ddf_fun_get_name(fun->fnode), irq);
}

void pci_read_interrupt(pci_fun_t *fun)
{
	uint8_t irq = pci_conf_read_8(fun, PCI_BRIDGE_INT_LINE);
	uint8_t pin = pci_conf_read_8(fun, PCI_BRIDGE_INT_PIN);

	if (pin != 0 && irq != 0xff)
		pci_add_interrupt(fun, irq);
}

/** Enumerate (recursively) and register the devices connected to a pci bus.
 *
 * @param bus		Host-to-PCI bridge
 * @param bus_num	Bus number
 */
void pci_bus_scan(pci_bus_t *bus, int bus_num)
{
	pci_fun_t *fun;
	errno_t rc;

	int child_bus = 0;
	int dnum, fnum;
	bool multi;
	uint8_t header_type;

	for (dnum = 0; dnum < 32; dnum++) {
		multi = true;
		for (fnum = 0; multi && fnum < 8; fnum++) {
			fun = pci_fun_new(bus);

			pci_fun_init(fun, bus_num, dnum, fnum);
			if (fun->vendor_id == 0xffff) {
				pci_fun_delete(fun);
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

			char *fun_name = pci_fun_create_name(fun);
			if (fun_name == NULL) {
				ddf_msg(LVL_ERROR, "Out of memory.");
				pci_fun_delete(fun);
				return;
			}

			rc = ddf_fun_set_name(fun->fnode, fun_name);
			free(fun_name);
			if (rc != EOK) {
				ddf_msg(LVL_ERROR, "Failed setting function name.");
				pci_fun_delete(fun);
				return;
			}

			pci_alloc_resource_list(fun);
			pci_read_bars(fun);
			pci_read_interrupt(fun);

			/* Propagate the PIO window to the function. */
			fun->pio_window = bus->pio_win;

			ddf_fun_set_ops(fun->fnode, &pci_fun_ops);

			ddf_msg(LVL_DEBUG, "Adding new function %s.",
			    ddf_fun_get_name(fun->fnode));

			pci_fun_create_match_ids(fun);

			if (ddf_fun_bind(fun->fnode) != EOK) {
				pci_clean_resource_list(fun);
				pci_fun_delete(fun);
				continue;
			}

			if (header_type == PCI_HEADER_TYPE_BRIDGE ||
			    header_type == PCI_HEADER_TYPE_CARDBUS) {
				child_bus = pci_conf_read_8(fun,
				    PCI_BRIDGE_SEC_BUS_NUM);
				ddf_msg(LVL_DEBUG, "Device is pci-to-pci "
				    "bridge, secondary bus number = %d.",
				    bus_num);
				if (child_bus > bus_num)
					pci_bus_scan(bus, child_bus);
			}
		}
	}
}

static errno_t pci_dev_add(ddf_dev_t *dnode)
{
	hw_resource_list_t hw_resources;
	pci_bus_t *bus = NULL;
	ddf_fun_t *ctl = NULL;
	bool got_res = false;
	async_sess_t *sess;
	errno_t rc;

	ddf_msg(LVL_DEBUG, "pci_dev_add");

	bus = ddf_dev_data_alloc(dnode, sizeof(pci_bus_t));
	if (bus == NULL) {
		ddf_msg(LVL_ERROR, "pci_dev_add allocation failed.");
		rc = ENOMEM;
		goto fail;
	}
	fibril_mutex_initialize(&bus->conf_mutex);

	bus->dnode = dnode;

	sess = ddf_dev_parent_sess_get(dnode);
	if (sess == NULL) {
		ddf_msg(LVL_ERROR, "pci_dev_add failed to connect to the "
		    "parent driver.");
		rc = ENOENT;
		goto fail;
	}

	rc = pio_window_get(sess, &bus->pio_win);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "pci_dev_add failed to get PIO window "
		    "for the device.");
		goto fail;
	}

	rc = hw_res_get_resource_list(sess, &hw_resources);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "pci_dev_add failed to get hw resources "
		    "for the device.");
		goto fail;
	}
	got_res = true;


	assert(hw_resources.count >= 1);

	if (hw_resources.count == 1) {
		assert(hw_resources.resources[0].type == MEM_RANGE);

		ddf_msg(LVL_DEBUG, "conf_addr_space = %" PRIx64 ".",
		    hw_resources.resources[0].res.mem_range.address);

		if (pio_enable_resource(&bus->pio_win,
		    &hw_resources.resources[0],
		    (void **) &bus->conf_space)) {
			ddf_msg(LVL_ERROR,
			    "Failed to map configuration space.");
			rc = EADDRNOTAVAIL;
			goto fail;
		}

	} else {
		assert(hw_resources.resources[0].type == IO_RANGE);
		assert(hw_resources.resources[0].res.io_range.size >= 4);

		assert(hw_resources.resources[1].type == IO_RANGE);
		assert(hw_resources.resources[1].res.io_range.size >= 4);

		ddf_msg(LVL_DEBUG, "conf_addr = %" PRIx64 ".",
		    hw_resources.resources[0].res.io_range.address);
		ddf_msg(LVL_DEBUG, "data_addr = %" PRIx64 ".",
		    hw_resources.resources[1].res.io_range.address);

		if (pio_enable_resource(&bus->pio_win,
		    &hw_resources.resources[0],
		    (void **) &bus->conf_addr_reg)) {
			ddf_msg(LVL_ERROR,
			    "Failed to enable configuration ports.");
			rc = EADDRNOTAVAIL;
			goto fail;
		}
		if (pio_enable_resource(&bus->pio_win,
		    &hw_resources.resources[1],
		    (void **) &bus->conf_data_reg)) {
			ddf_msg(LVL_ERROR,
			    "Failed to enable configuration ports.");
			rc = EADDRNOTAVAIL;
			goto fail;
		}
	}

	/* Make the bus device more visible. It has no use yet. */
	ddf_msg(LVL_DEBUG, "Adding a 'ctl' function");

	ctl = ddf_fun_create(bus->dnode, fun_exposed, "ctl");
	if (ctl == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating control function.");
		rc = ENOMEM;
		goto fail;
	}

	rc = ddf_fun_bind(ctl);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding control function.");
		goto fail;
	}

	/* Enumerate functions. */
	ddf_msg(LVL_DEBUG, "Scanning the bus");
	pci_bus_scan(bus, 0);

	hw_res_clean_resource_list(&hw_resources);

	return EOK;

fail:
	if (got_res)
		hw_res_clean_resource_list(&hw_resources);

	if (ctl != NULL)
		ddf_fun_destroy(ctl);

	return rc;
}

static errno_t pci_fun_online(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "pci_fun_online()");
	return ddf_fun_online(fun);
}

static errno_t pci_fun_offline(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "pci_fun_offline()");
	return ddf_fun_offline(fun);
}

static void pciintel_init(void)
{
	ddf_log_init(NAME);
}

pci_fun_t *pci_fun_new(pci_bus_t *bus)
{
	pci_fun_t *fun;
	ddf_fun_t *fnode;

	fnode = ddf_fun_create(bus->dnode, fun_inner, NULL);
	if (fnode == NULL)
		return NULL;

	fun = ddf_fun_data_alloc(fnode, sizeof(pci_fun_t));
	if (fun == NULL)
		return NULL;

	fun->busptr = bus;
	fun->fnode = fnode;
	return fun;
}

void pci_fun_init(pci_fun_t *fun, int bus, int dev, int fn)
{
	fun->bus = bus;
	fun->dev = dev;
	fun->fn = fn;
	fun->vendor_id = pci_conf_read_16(fun, PCI_VENDOR_ID);
	fun->device_id = pci_conf_read_16(fun, PCI_DEVICE_ID);

	/* Explicitly enable PCI bus mastering */
	fun->command = pci_conf_read_16(fun, PCI_COMMAND) |
	    PCI_COMMAND_MASTER;
	pci_conf_write_16(fun, PCI_COMMAND, fun->command);

	fun->class_code = pci_conf_read_8(fun, PCI_BASE_CLASS);
	fun->subclass_code = pci_conf_read_8(fun, PCI_SUB_CLASS);
	fun->prog_if = pci_conf_read_8(fun, PCI_PROG_IF);
	fun->revision = pci_conf_read_8(fun, PCI_REVISION_ID);
}

void pci_fun_delete(pci_fun_t *fun)
{
	hw_res_clean_resource_list(&fun->hw_resources);
	if (fun->fnode != NULL)
		ddf_fun_destroy(fun->fnode);
}

char *pci_fun_create_name(pci_fun_t *fun)
{
	char *name = NULL;

	asprintf(&name, "%02x:%02x.%01x", fun->bus, fun->dev,
	    fun->fn);
	return name;
}

bool pci_alloc_resource_list(pci_fun_t *fun)
{
	fun->hw_resources.resources = fun->resources;
	return true;
}

void pci_clean_resource_list(pci_fun_t *fun)
{
	fun->hw_resources.resources = NULL;
}

/** Read the base address registers (BARs) of the function and add the addresses
 * to its HW resource list.
 *
 * @param fun	PCI function
 */
void pci_read_bars(pci_fun_t *fun)
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
	size_t size = mask & ~(mask - 1);
	return size;
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS PCI bus driver (Intel method 1).\n");
	pciintel_init();
	return ddf_driver_main(&pci_driver);
}

/**
 * @}
 */
