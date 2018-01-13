/*
 * Copyright (c) 2011 Martin Decky
 * Copyright (c) 2017 Jiri Svoboda
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
 * @defgroup mac Mac platform driver.
 * @brief HelenOS Mac platform driver.
 * @{
 */

/** @file
 */

#include <ddf/driver.h>
#include <ddf/log.h>
#include <errno.h>
#include <ops/hw_res.h>
#include <ops/pio_window.h>
#include <stdio.h>
#include <sysinfo.h>

#define NAME  "mac"

typedef struct {
	hw_resource_list_t hw_resources;
	pio_window_t pio_window;
} mac_fun_t;

static hw_resource_t adb_res[] = {
	{
		.type = IO_RANGE,
		.res.io_range = {
			.address = 0,
			.size = 0x2000,
			.relative = true,
			.endianness = BIG_ENDIAN
		}
	},
	{
		.type = INTERRUPT,
		.res.interrupt = {
			.irq = 0 /* patched at run time */
		}
	},
};

static mac_fun_t adb_data = {
	.hw_resources = {
		sizeof(adb_res) / sizeof(adb_res[0]),
		adb_res
	},
	.pio_window = {
		.io = {
			.base = 0, /* patched at run time */
			.size = 0x2000
		}
	}
};

static hw_resource_t pci_conf_regs[] = {
	{
		.type = IO_RANGE,
		.res.io_range = {
			.address = 0xfec00000,
			.size = 4,
			.relative = false,
			.endianness = LITTLE_ENDIAN
		}
	},
	{
		.type = IO_RANGE,
		.res.io_range = {
			.address = 0xfee00000,
			.size = 4,
			.relative = false,
			.endianness = LITTLE_ENDIAN
		}
	}
};

static mac_fun_t pci_data = {
	.hw_resources = {
		2,
		pci_conf_regs
	}
};

static ddf_dev_ops_t mac_fun_ops;

/** Obtain function soft-state from DDF function node */
static mac_fun_t *mac_fun(ddf_fun_t *ddf_fun)
{
	return ddf_fun_data_get(ddf_fun);
}

static pio_window_t *mac_get_pio_window(ddf_fun_t *ddf_fun)
{
	mac_fun_t *fun = mac_fun(ddf_fun);
	return &fun->pio_window;
}

static bool mac_add_fun(ddf_dev_t *dev, const char *name,
    const char *str_match_id, mac_fun_t *fun_proto)
{
	ddf_msg(LVL_DEBUG, "Adding new function '%s'.", name);
	printf("mac: Adding new function '%s'.\n", name);
	
	ddf_fun_t *fnode = NULL;
	errno_t rc;
	
	/* Create new device. */
	fnode = ddf_fun_create(dev, fun_inner, name);
	if (fnode == NULL)
		goto failure;
	
	mac_fun_t *fun = ddf_fun_data_alloc(fnode, sizeof(mac_fun_t));
	*fun = *fun_proto;
	
	/* Add match ID */
	rc = ddf_fun_add_match_id(fnode, str_match_id, 100);
	if (rc != EOK)
		goto failure;
	
	/* Set provided operations to the device. */
	ddf_fun_set_ops(fnode, &mac_fun_ops);
	
	/* Register function. */
	if (ddf_fun_bind(fnode) != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function %s.", name);
		goto failure;
	}
	
	printf("mac: Added new function '%s' (str=%s).\n", name, str_match_id);
	return true;
	
failure:
	if (fnode != NULL)
		ddf_fun_destroy(fnode);
	
	ddf_msg(LVL_ERROR, "Failed adding function '%s'.", name);
	
	return false;
}

/** Get the root device.
 *
 * @param dev Device which is root of the whole device tree
 *            (both of HW and pseudo devices).
 *
 * @return Zero on success, error number otherwise.
 *
 */
static errno_t mac_dev_add(ddf_dev_t *dev)
{
	errno_t rc;
	uintptr_t cuda_physical;
	sysarg_t cuda_inr;
#if 0
	/* Register functions */
	if (!mac_add_fun(dev, "pci0", "intel_pci", &pci_data)) {
		ddf_msg(LVL_ERROR, "Failed to add PCI function for Mac platform.");
		return EIO;
	}
#else
	(void)pci_data;
#endif
	rc = sysinfo_get_value("cuda.address.physical", &cuda_physical);
	if (rc != EOK)
		return EIO;
	rc = sysinfo_get_value("cuda.inr", &cuda_inr);
	if (rc != EOK)
		return EIO;

	adb_data.pio_window.io.base = cuda_physical;
	adb_res[1].res.interrupt.irq = cuda_inr;

	if (!mac_add_fun(dev, "adb", "cuda_adb", &adb_data)) {
		ddf_msg(LVL_ERROR, "Failed to add ADB function for Mac platform.");
		return EIO;
	}

	return EOK;
}

/** The root device driver's standard operations. */
static driver_ops_t mac_ops = {
	.dev_add = &mac_dev_add
};

/** The root device driver structure. */
static driver_t mac_driver = {
	.name = NAME,
	.driver_ops = &mac_ops
};

static hw_resource_list_t *mac_get_resources(ddf_fun_t *fnode)
{
	mac_fun_t *fun = mac_fun(fnode);
	assert(fun != NULL);
	
	return &fun->hw_resources;
}

static errno_t mac_enable_interrupt(ddf_fun_t *fun, int irq)
{
	/* TODO */
	
	return false;
}

static pio_window_ops_t fun_pio_window_ops = {
        .get_pio_window = &mac_get_pio_window
};

static hw_res_ops_t fun_hw_res_ops = {
   	.get_resource_list = &mac_get_resources,
	.enable_interrupt = &mac_enable_interrupt
};

int main(int argc, char *argv[])
{
	printf("%s: HelenOS Mac platform driver\n", NAME);
	ddf_log_init(NAME);
	mac_fun_ops.interfaces[HW_RES_DEV_IFACE] = &fun_hw_res_ops;
	mac_fun_ops.interfaces[PIO_WINDOW_DEV_IFACE] = &fun_pio_window_ops;
	return ddf_driver_main(&mac_driver);
}

/**
 * @}
 */
