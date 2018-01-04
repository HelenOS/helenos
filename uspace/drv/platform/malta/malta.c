/*
 * Copyright (c) 2010 Lenka Trochtova
 * Copyright (c) 2013 Jakub Jermar
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
 * @defgroup malta Malta board platform driver.
 * @brief HelenOS Malta board platform driver.
 * @{
 */

/** @file
 */

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <fibril_synch.h>
#include <stdlib.h>
#include <str.h>
#include <ctype.h>
#include <macros.h>

#include <ddi.h>
#include <ddf/driver.h>
#include <ddf/log.h>
#include <ipc/dev_iface.h>
#include <ops/hw_res.h>
#include <ops/pio_window.h>
#include <byteorder.h>

#define NAME "malta"

#define GT_BASE		UINT32_C(0x1be00000)
#define GT_SIZE 	(2 * 1024 * 1024)

#define GT_PCI_CMD	0xc00
#define GT_PCI_CONFADDR	0xcf8
#define GT_PCI_CONFDATA	0xcfc

#define GT_PCI_CMD_MBYTESWAP	0x1

#define GT_PCI_MEMBASE	UINT32_C(0x10000000)
#define GT_PCI_MEMSIZE	UINT32_C(0x08000000)

#define GT_PCI_IOBASE	UINT32_C(0x18000000)
#define GT_PCI_IOSIZE	UINT32_C(0x00200000)

typedef struct malta_fun {
	hw_resource_list_t hw_resources;
	pio_window_t pio_window;
} malta_fun_t;

static errno_t malta_dev_add(ddf_dev_t *dev);
static void malta_init(void);

/** The root device driver's standard operations. */
static driver_ops_t malta_ops = {
	.dev_add = &malta_dev_add
};

/** The root device driver structure. */
static driver_t malta_driver = {
	.name = NAME,
	.driver_ops = &malta_ops
};

static hw_resource_t pci_conf_regs[] = {
	{
		.type = IO_RANGE,
		.res.io_range = {
			.address = GT_BASE + GT_PCI_CONFADDR,
			.size = 4,
			.relative = false,
			.endianness = LITTLE_ENDIAN
		}
	},
	{
		.type = IO_RANGE,
		.res.io_range = {
			.address = GT_BASE + GT_PCI_CONFDATA,
			.size = 4,
			.relative = false,
			.endianness = LITTLE_ENDIAN
		}
	}
};

static malta_fun_t pci_data = {
	.hw_resources = {
		sizeof(pci_conf_regs) / sizeof(pci_conf_regs[0]),
		pci_conf_regs
	},
	.pio_window = {
		.mem = {
			.base = GT_PCI_MEMBASE,
			.size = GT_PCI_MEMSIZE
		},
		.io = {
			.base = GT_PCI_IOBASE,
			.size = GT_PCI_IOSIZE
		}
	}
};

/** Obtain function soft-state from DDF function node */
static malta_fun_t *malta_fun(ddf_fun_t *fnode)
{
	return ddf_fun_data_get(fnode);
}

static hw_resource_list_t *malta_get_resources(ddf_fun_t *fnode)
{
	malta_fun_t *fun = malta_fun(fnode);
	
	assert(fun != NULL);
	return &fun->hw_resources;
}

static errno_t malta_enable_interrupt(ddf_fun_t *fun, int irq)
{
	/* TODO */
	
	return false;
}

static pio_window_t *malta_get_pio_window(ddf_fun_t *fnode)
{
	malta_fun_t *fun = malta_fun(fnode);

	assert(fun != NULL);
	return &fun->pio_window;
}

static hw_res_ops_t fun_hw_res_ops = {
	.get_resource_list = &malta_get_resources,
	.enable_interrupt = &malta_enable_interrupt,
};

static pio_window_ops_t fun_pio_window_ops = {
	.get_pio_window = &malta_get_pio_window
};

/* Initialized in malta_init() function. */
static ddf_dev_ops_t malta_fun_ops;

static bool
malta_add_fun(ddf_dev_t *dev, const char *name, const char *str_match_id,
    malta_fun_t *fun_proto)
{
	ddf_msg(LVL_DEBUG, "Adding new function '%s'.", name);
	
	ddf_fun_t *fnode = NULL;
	errno_t rc;
	
	/* Create new device. */
	fnode = ddf_fun_create(dev, fun_inner, name);
	if (fnode == NULL)
		goto failure;
	
	malta_fun_t *fun = ddf_fun_data_alloc(fnode, sizeof(malta_fun_t));
	*fun = *fun_proto;
	
	/* Add match ID */
	rc = ddf_fun_add_match_id(fnode, str_match_id, 100);
	if (rc != EOK)
		goto failure;
	
	/* Set provided operations to the device. */
	ddf_fun_set_ops(fnode, &malta_fun_ops);
	
	/* Register function. */
	if (ddf_fun_bind(fnode) != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function %s.", name);
		goto failure;
	}
	
	return true;
	
failure:
	if (fnode != NULL)
		ddf_fun_destroy(fnode);
	
	ddf_msg(LVL_ERROR, "Failed adding function '%s'.", name);
	
	return false;
}

static bool malta_add_functions(ddf_dev_t *dev)
{
	return malta_add_fun(dev, "pci0", "intel_pci", &pci_data);
}

/** Get the root device.
 *
 * @param dev		The device which is root of the whole device tree (both
 *			of HW and pseudo devices).
 * @return		Zero on success, error number otherwise.
 */
static errno_t malta_dev_add(ddf_dev_t *dev)
{
	ioport32_t *gt;
	uint32_t val;
	errno_t ret;

	ddf_msg(LVL_DEBUG, "malta_dev_add, device handle = %d",
	    (int)ddf_dev_get_handle(dev));

	/*
 	 * We need to disable byte swapping of the outgoing and incoming
 	 * PCI data, because the PCI driver assumes no byte swapping behind
 	 * the scenes and takes care of it itself.
 	 */
	ret = pio_enable((void *) GT_BASE, GT_SIZE, (void **) &gt);
	if (ret != EOK)
                return ret;
	val = uint32_t_le2host(pio_read_32(
	    &gt[GT_PCI_CMD / sizeof(ioport32_t)]));
	val |= GT_PCI_CMD_MBYTESWAP;
	pio_write_32(
	    &gt[GT_PCI_CMD / sizeof(ioport32_t)], host2uint32_t_le(val));

	
	/* Register functions. */
	if (!malta_add_functions(dev)) {
		ddf_msg(LVL_ERROR, "Failed to add functions for the Malta platform.");
	}
	
	return EOK;
}

static void malta_init(void)
{
	ddf_log_init(NAME);
	malta_fun_ops.interfaces[HW_RES_DEV_IFACE] = &fun_hw_res_ops;
	malta_fun_ops.interfaces[PIO_WINDOW_DEV_IFACE] = &fun_pio_window_ops;
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS Malta platform driver\n");
	malta_init();
	return ddf_driver_main(&malta_driver);
}

/**
 * @}
 */
