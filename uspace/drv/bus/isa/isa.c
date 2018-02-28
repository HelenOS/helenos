/*
 * Copyright (c) 2010 Lenka Trochtova
 * Copyright (c) 2011 Jiri Svoboda
 * Copyright (c) 2011 Jan Vesely
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
 * @defgroup isa ISA bus driver.
 * @brief HelenOS ISA bus driver.
 * @{
 */

/** @file
 */

#include <adt/list.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <fibril_synch.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>
#include <ctype.h>
#include <macros.h>
#include <stdlib.h>
#include <dirent.h>
#include <ipc/irc.h>
#include <ipc/services.h>
#include <vfs/vfs.h>
#include <irc.h>
#include <ns.h>

#include <ddf/driver.h>
#include <ddf/log.h>
#include <ops/hw_res.h>
#include <ops/pio_window.h>

#include <device/hw_res.h>
#include <device/pio_window.h>

#include <pci_dev_iface.h>

#include "i8237.h"

#define NAME "isa"
#define ISA_CHILD_FUN_CONF_PATH "/drv/isa/isa.dev"
#define EBUS_CHILD_FUN_CONF_PATH "/drv/isa/ebus.dev"

#define ISA_MAX_HW_RES 5

typedef struct {
	fibril_mutex_t mutex;
	uint16_t pci_vendor_id;
	uint16_t pci_device_id;
	uint8_t pci_class;
	uint8_t pci_subclass;
	ddf_dev_t *dev;
	ddf_fun_t *fctl;
	pio_window_t pio_win;
	list_t functions;
} isa_bus_t;

typedef struct isa_fun {
	fibril_mutex_t mutex;
	ddf_fun_t *fnode;
	hw_resource_t resources[ISA_MAX_HW_RES];
	hw_resource_list_t hw_resources;
	link_t bus_link;
} isa_fun_t;

/** Obtain soft-state from device node */
static isa_bus_t *isa_bus(ddf_dev_t *dev)
{
	return ddf_dev_data_get(dev);
}

/** Obtain soft-state from function node */
static isa_fun_t *isa_fun(ddf_fun_t *fun)
{
	return ddf_fun_data_get(fun);
}

static hw_resource_list_t *isa_fun_get_resources(ddf_fun_t *fnode)
{
	isa_fun_t *fun = isa_fun(fnode);
	assert(fun);

	return &fun->hw_resources;
}

static bool isa_fun_owns_interrupt(isa_fun_t *fun, int irq)
{
	const hw_resource_list_t *res = &fun->hw_resources;

	/* Check that specified irq really belongs to the function */
	for (size_t i = 0; i < res->count; ++i) {
		if (res->resources[i].type == INTERRUPT &&
		    res->resources[i].res.interrupt.irq == irq) {
			return true;
		}
	}

	return false;
}

static errno_t isa_fun_enable_interrupt(ddf_fun_t *fnode, int irq)
{
	isa_fun_t *fun = isa_fun(fnode);

	if (!isa_fun_owns_interrupt(fun, irq))
		return EINVAL;

	return irc_enable_interrupt(irq);
}

static errno_t isa_fun_disable_interrupt(ddf_fun_t *fnode, int irq)
{
	isa_fun_t *fun = isa_fun(fnode);

	if (!isa_fun_owns_interrupt(fun, irq))
		return EINVAL;

	return irc_disable_interrupt(irq);
}

static errno_t isa_fun_clear_interrupt(ddf_fun_t *fnode, int irq)
{
	isa_fun_t *fun = isa_fun(fnode);

	if (!isa_fun_owns_interrupt(fun, irq))
		return EINVAL;

	return irc_clear_interrupt(irq);
}

static errno_t isa_fun_setup_dma(ddf_fun_t *fnode,
    unsigned int channel, uint32_t pa, uint32_t size, uint8_t mode)
{
	assert(fnode);
	isa_fun_t *fun = isa_fun(fnode);
	assert(fun);
	const hw_resource_list_t *res = &fun->hw_resources;
	assert(res);

	for (size_t i = 0; i < res->count; ++i) {
		/* Check for assigned channel */
		if (((res->resources[i].type == DMA_CHANNEL_16) &&
		    (res->resources[i].res.dma_channel.dma16 == channel)) ||
		    ((res->resources[i].type == DMA_CHANNEL_8) &&
		    (res->resources[i].res.dma_channel.dma8 == channel))) {
			return dma_channel_setup(channel, pa, size, mode);
		}
	}

	return EINVAL;
}

static errno_t isa_fun_remain_dma(ddf_fun_t *fnode,
    unsigned channel, size_t *size)
{
	assert(size);
	assert(fnode);
	isa_fun_t *fun = isa_fun(fnode);
	assert(fun);
	const hw_resource_list_t *res = &fun->hw_resources;
	assert(res);

	for (size_t i = 0; i < res->count; ++i) {
		/* Check for assigned channel */
		if (((res->resources[i].type == DMA_CHANNEL_16) &&
		    (res->resources[i].res.dma_channel.dma16 == channel)) ||
		    ((res->resources[i].type == DMA_CHANNEL_8) &&
		    (res->resources[i].res.dma_channel.dma8 == channel))) {
			return dma_channel_remain(channel, size);
		}
	}

	return EINVAL;
}

static hw_res_ops_t isa_fun_hw_res_ops = {
	.get_resource_list = isa_fun_get_resources,
	.enable_interrupt = isa_fun_enable_interrupt,
	.disable_interrupt = isa_fun_disable_interrupt,
	.clear_interrupt = isa_fun_clear_interrupt,
	.dma_channel_setup = isa_fun_setup_dma,
	.dma_channel_remain = isa_fun_remain_dma,
};

static pio_window_t *isa_fun_get_pio_window(ddf_fun_t *fnode)
{
	ddf_dev_t *dev = ddf_fun_get_dev(fnode);
	isa_bus_t *isa = isa_bus(dev);
	assert(isa);

	return &isa->pio_win;
}

static pio_window_ops_t isa_fun_pio_window_ops = {
	.get_pio_window = isa_fun_get_pio_window
};

static ddf_dev_ops_t isa_fun_ops= {
	.interfaces[HW_RES_DEV_IFACE] = &isa_fun_hw_res_ops,
	.interfaces[PIO_WINDOW_DEV_IFACE] = &isa_fun_pio_window_ops,
};

static errno_t isa_dev_add(ddf_dev_t *dev);
static errno_t isa_dev_remove(ddf_dev_t *dev);
static errno_t isa_fun_online(ddf_fun_t *fun);
static errno_t isa_fun_offline(ddf_fun_t *fun);

/** The isa device driver's standard operations */
static driver_ops_t isa_ops = {
	.dev_add = &isa_dev_add,
	.dev_remove = &isa_dev_remove,
	.fun_online = &isa_fun_online,
	.fun_offline = &isa_fun_offline
};

/** The isa device driver structure. */
static driver_t isa_driver = {
	.name = NAME,
	.driver_ops = &isa_ops
};

static isa_fun_t *isa_fun_create(isa_bus_t *isa, const char *name)
{
	ddf_fun_t *fnode = ddf_fun_create(isa->dev, fun_inner, name);
	if (fnode == NULL)
		return NULL;

	isa_fun_t *fun = ddf_fun_data_alloc(fnode, sizeof(isa_fun_t));
	if (fun == NULL) {
		ddf_fun_destroy(fnode);
		return NULL;
	}

	fibril_mutex_initialize(&fun->mutex);
	fun->hw_resources.resources = fun->resources;

	fun->fnode = fnode;
	return fun;
}

static char *fun_conf_read(const char *conf_path)
{
	bool suc = false;
	char *buf = NULL;
	bool opened = false;
	int fd;
	size_t len;
	errno_t rc;
	size_t nread;
	vfs_stat_t st;

	rc = vfs_lookup_open(conf_path, WALK_REGULAR, MODE_READ, &fd);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Unable to open %s", conf_path);
		goto cleanup;
	}

	opened = true;

	if (vfs_stat(fd, &st) != EOK) {
		ddf_msg(LVL_ERROR, "Unable to vfs_stat %d", fd);
		goto cleanup;
	}

	len = st.size;
	if (len == 0) {
		ddf_msg(LVL_ERROR, "Configuration file '%s' is empty.",
		    conf_path);
		goto cleanup;
	}

	buf = malloc(len + 1);
	if (buf == NULL) {
		ddf_msg(LVL_ERROR, "Memory allocation failed.");
		goto cleanup;
	}

	rc = vfs_read(fd, (aoff64_t []) {0}, buf, len, &nread);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Unable to read file '%s'.", conf_path);
		goto cleanup;
	}

	buf[nread] = 0;

	suc = true;

cleanup:
	if (!suc && buf != NULL) {
		free(buf);
		buf = NULL;
	}

	if (opened)
		vfs_put(fd);

	return buf;
}

static char *str_get_line(char *str, char **next)
{
	char *line = str;
	*next = NULL;

	if (str == NULL) {
		return NULL;
	}

	while (*str != '\0' && *str != '\n') {
		str++;
	}

	if (*str != '\0') {
		*next = str + 1;
	}

	*str = '\0';
	return line;
}

static bool line_empty(const char *line)
{
	while (line != NULL && *line != 0) {
		if (!isspace(*line))
			return false;
		line++;
	}

	return true;
}

static char *get_device_name(char *line)
{
	/* Skip leading spaces. */
	while (*line != '\0' && isspace(*line)) {
		line++;
	}

	/* Get the name part of the rest of the line. */
	str_tok(line, ":", NULL);
	return line;
}

static inline const char *skip_spaces(const char *line)
{
	/* Skip leading spaces. */
	while (*line != '\0' && isspace(*line))
		line++;

	return line;
}

static void isa_fun_add_irq(isa_fun_t *fun, int irq)
{
	size_t count = fun->hw_resources.count;
	hw_resource_t *resources = fun->hw_resources.resources;

	if (count < ISA_MAX_HW_RES) {
		resources[count].type = INTERRUPT;
		resources[count].res.interrupt.irq = irq;

		fun->hw_resources.count++;

		ddf_msg(LVL_NOTE, "Added irq 0x%x to function %s", irq,
		    ddf_fun_get_name(fun->fnode));
	}
}

static void isa_fun_add_dma(isa_fun_t *fun, int dma)
{
	size_t count = fun->hw_resources.count;
	hw_resource_t *resources = fun->hw_resources.resources;

	if (count < ISA_MAX_HW_RES) {
		if ((dma > 0) && (dma < 4)) {
			resources[count].type = DMA_CHANNEL_8;
			resources[count].res.dma_channel.dma8 = dma;

			fun->hw_resources.count++;
			ddf_msg(LVL_NOTE, "Added dma 0x%x to function %s", dma,
			    ddf_fun_get_name(fun->fnode));

			return;
		}

		if ((dma > 4) && (dma < 8)) {
			resources[count].type = DMA_CHANNEL_16;
			resources[count].res.dma_channel.dma16 = dma;

			fun->hw_resources.count++;
			ddf_msg(LVL_NOTE, "Added dma 0x%x to function %s", dma,
			    ddf_fun_get_name(fun->fnode));

			return;
		}

		ddf_msg(LVL_WARN, "Skipped dma 0x%x for function %s", dma,
		    ddf_fun_get_name(fun->fnode));
	}
}

static void isa_fun_add_io_range(isa_fun_t *fun, size_t addr, size_t len)
{
	size_t count = fun->hw_resources.count;
	hw_resource_t *resources = fun->hw_resources.resources;

	isa_bus_t *isa = isa_bus(ddf_fun_get_dev(fun->fnode));

	if (count < ISA_MAX_HW_RES) {
		resources[count].type = IO_RANGE;
		resources[count].res.io_range.address = addr;
		resources[count].res.io_range.address += isa->pio_win.io.base;
		resources[count].res.io_range.size = len;
		resources[count].res.io_range.relative = false;
		resources[count].res.io_range.endianness = LITTLE_ENDIAN;

		fun->hw_resources.count++;

		ddf_msg(LVL_NOTE, "Added io range (addr=0x%x, size=0x%x) to "
		    "function %s", (unsigned int) addr, (unsigned int) len,
		    ddf_fun_get_name(fun->fnode));
	}
}

static void isa_fun_add_mem_range(isa_fun_t *fun, uintptr_t addr, size_t len)
{
	size_t count = fun->hw_resources.count;
	hw_resource_t *resources = fun->hw_resources.resources;

	isa_bus_t *isa = isa_bus(ddf_fun_get_dev(fun->fnode));

	if (count < ISA_MAX_HW_RES) {
		resources[count].type = MEM_RANGE;
		resources[count].res.mem_range.address = addr;
		resources[count].res.mem_range.address += isa->pio_win.mem.base;
		resources[count].res.mem_range.size = len;
		resources[count].res.mem_range.relative = true;
		resources[count].res.mem_range.endianness = LITTLE_ENDIAN;

		fun->hw_resources.count++;

		ddf_msg(LVL_NOTE, "Added mem range (addr=0x%zx, size=0x%x) to "
		    "function %s", (uintptr_t) addr, (unsigned int) len,
		    ddf_fun_get_name(fun->fnode));
	}
}

static void fun_parse_irq(isa_fun_t *fun, const char *val)
{
	int irq = 0;
	char *end = NULL;

	val = skip_spaces(val);
	irq = (int) strtol(val, &end, 10);

	if (val != end)
		isa_fun_add_irq(fun, irq);
}

static void fun_parse_dma(isa_fun_t *fun, const char *val)
{
	char *end = NULL;

	val = skip_spaces(val);
	const int dma = strtol(val, &end, 10);

	if (val != end)
		isa_fun_add_dma(fun, dma);
}

static void fun_parse_io_range(isa_fun_t *fun, const char *val)
{
	size_t addr, len;
	char *end = NULL;

	val = skip_spaces(val);
	addr = strtol(val, &end, 0x10);

	if (val == end)
		return;

	val = skip_spaces(end);
	len = strtol(val, &end, 0x10);

	if (val == end)
		return;

	isa_fun_add_io_range(fun, addr, len);
}

static void fun_parse_mem_range(isa_fun_t *fun, const char *val)
{
	uintptr_t addr;
	size_t len;
	char *end = NULL;

	val = skip_spaces(val);
	addr = strtoul(val, &end, 0x10);

	if (val == end)
		return;

	val = skip_spaces(end);
	len = strtol(val, &end, 0x10);

	if (val == end)
		return;

	isa_fun_add_mem_range(fun, addr, len);
}

static void get_match_id(char **id, const char *val)
{
	const char *end = val;

	while (!isspace(*end))
		end++;

	size_t size = end - val + 1;
	*id = (char *)malloc(size);
	str_cpy(*id, size, val);
}

static void fun_parse_match_id(isa_fun_t *fun, const char *val)
{
	char *id = NULL;
	char *end = NULL;

	val = skip_spaces(val);

	int score = (int)strtol(val, &end, 10);
	if (val == end) {
		ddf_msg(LVL_ERROR, "Cannot read match score for function "
		    "%s.", ddf_fun_get_name(fun->fnode));
		return;
	}

	val = skip_spaces(end);
	get_match_id(&id, val);
	if (id == NULL) {
		ddf_msg(LVL_ERROR, "Cannot read match ID for function %s.",
		    ddf_fun_get_name(fun->fnode));
		return;
	}

	ddf_msg(LVL_DEBUG, "Adding match id '%s' with score %d to "
	    "function %s", id, score, ddf_fun_get_name(fun->fnode));

	errno_t rc = ddf_fun_add_match_id(fun->fnode, id, score);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed adding match ID: %s",
		    str_error(rc));
	}

	free(id);
}

static bool prop_parse(isa_fun_t *fun, const char *line, const char *prop,
    void (*read_fn)(isa_fun_t *, const char *))
{
	size_t proplen = str_size(prop);

	if (str_lcmp(line, prop, proplen) == 0) {
		line += proplen;
		line = skip_spaces(line);
		(*read_fn)(fun, line);

		return true;
	}

	return false;
}

static void fun_prop_parse(isa_fun_t *fun, const char *line)
{
	/* Skip leading spaces. */
	line = skip_spaces(line);

	if (!prop_parse(fun, line, "io_range", &fun_parse_io_range) &&
	    !prop_parse(fun, line, "mem_range", &fun_parse_mem_range) &&
	    !prop_parse(fun, line, "irq", &fun_parse_irq) &&
	    !prop_parse(fun, line, "dma", &fun_parse_dma) &&
	    !prop_parse(fun, line, "match", &fun_parse_match_id)) {

		ddf_msg(LVL_ERROR, "Undefined device property at line '%s'",
		    line);
	}
}

static char *isa_fun_read_info(char *fun_conf, isa_bus_t *isa)
{
	char *line;

	/* Skip empty lines. */
	do {
		line = str_get_line(fun_conf, &fun_conf);

		if (line == NULL) {
			/* no more lines */
			return NULL;
		}

	} while (line_empty(line));

	/* Get device name. */
	const char *fun_name = get_device_name(line);
	if (fun_name == NULL)
		return NULL;

	isa_fun_t *fun = isa_fun_create(isa, fun_name);
	if (fun == NULL) {
		return NULL;
	}

	/* Get properties of the device (match ids, irq and io range). */
	while (true) {
		line = str_get_line(fun_conf, &fun_conf);

		if (line_empty(line)) {
			/* no more device properties */
			break;
		}

		/*
		 * Get the device's property from the configuration line
		 * and store it in the device structure.
		 */
		fun_prop_parse(fun, line);
	}

	/* Set device operations to the device. */
	ddf_fun_set_ops(fun->fnode, &isa_fun_ops);

	ddf_msg(LVL_DEBUG, "Binding function %s.", ddf_fun_get_name(fun->fnode));

	/* XXX Handle error */
	(void) ddf_fun_bind(fun->fnode);

	list_append(&fun->bus_link, &isa->functions);

	return fun_conf;
}

static void isa_functions_add(isa_bus_t *isa)
{
#define BASE_CLASS_BRIDGE	0x06
#define SUB_CLASS_BRIDGE_ISA	0x01
	bool isa_bridge = ((isa->pci_class == BASE_CLASS_BRIDGE) &&
	    (isa->pci_subclass == SUB_CLASS_BRIDGE_ISA));

#define VENDOR_ID_SUN	0x108e
#define DEVICE_ID_EBUS	0x1000
	bool ebus = ((isa->pci_vendor_id == VENDOR_ID_SUN) &&
	    (isa->pci_device_id == DEVICE_ID_EBUS));

	const char *conf_path = NULL;
	if (isa_bridge)
		conf_path = ISA_CHILD_FUN_CONF_PATH;
	else if (ebus)
		conf_path = EBUS_CHILD_FUN_CONF_PATH;

	char *conf = fun_conf_read(conf_path);
	while (conf != NULL && *conf != '\0') {
		conf = isa_fun_read_info(conf, isa);
	}
	free(conf);
}

static errno_t isa_dev_add(ddf_dev_t *dev)
{
	async_sess_t *sess;
	errno_t rc;

	ddf_msg(LVL_DEBUG, "isa_dev_add, device handle = %d",
	    (int) ddf_dev_get_handle(dev));

	isa_bus_t *isa = ddf_dev_data_alloc(dev, sizeof(isa_bus_t));
	if (isa == NULL)
		return ENOMEM;

	fibril_mutex_initialize(&isa->mutex);
	isa->dev = dev;
	list_initialize(&isa->functions);

	sess = ddf_dev_parent_sess_get(dev);
	if (sess == NULL) {
		ddf_msg(LVL_ERROR, "isa_dev_add failed to connect to the "
		    "parent driver.");
		return ENOENT;
	}

	rc = pci_config_space_read_16(sess, PCI_VENDOR_ID, &isa->pci_vendor_id);
	if (rc != EOK)
		return rc;
	rc = pci_config_space_read_16(sess, PCI_DEVICE_ID, &isa->pci_device_id);
	if (rc != EOK)
		return rc;
	rc = pci_config_space_read_8(sess, PCI_BASE_CLASS, &isa->pci_class);
	if (rc != EOK)
		return rc;
	rc = pci_config_space_read_8(sess, PCI_SUB_CLASS, &isa->pci_subclass);
	if (rc != EOK)
		return rc;

	rc = pio_window_get(sess, &isa->pio_win);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "isa_dev_add failed to get PIO window "
		    "for the device.");
		return rc;
	}

	/* Make the bus device more visible. Does not do anything. */
	ddf_msg(LVL_DEBUG, "Adding a 'ctl' function");

	fibril_mutex_lock(&isa->mutex);

	isa->fctl = ddf_fun_create(dev, fun_exposed, "ctl");
	if (isa->fctl == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating control function.");
		return EXDEV;
	}

	if (ddf_fun_bind(isa->fctl) != EOK) {
		ddf_fun_destroy(isa->fctl);
		ddf_msg(LVL_ERROR, "Failed binding control function.");
		return EXDEV;
	}

	/* Add functions as specified in the configuration file. */
	isa_functions_add(isa);
	ddf_msg(LVL_NOTE, "Finished enumerating legacy functions");

	fibril_mutex_unlock(&isa->mutex);

	return EOK;
}

static errno_t isa_dev_remove(ddf_dev_t *dev)
{
	isa_bus_t *isa = isa_bus(dev);

	fibril_mutex_lock(&isa->mutex);

	while (!list_empty(&isa->functions)) {
		isa_fun_t *fun = list_get_instance(list_first(&isa->functions),
		    isa_fun_t, bus_link);

		errno_t rc = ddf_fun_offline(fun->fnode);
		if (rc != EOK) {
			fibril_mutex_unlock(&isa->mutex);
			ddf_msg(LVL_ERROR, "Failed offlining %s", ddf_fun_get_name(fun->fnode));
			return rc;
		}

		rc = ddf_fun_unbind(fun->fnode);
		if (rc != EOK) {
			fibril_mutex_unlock(&isa->mutex);
			ddf_msg(LVL_ERROR, "Failed unbinding %s", ddf_fun_get_name(fun->fnode));
			return rc;
		}

		list_remove(&fun->bus_link);

		ddf_fun_destroy(fun->fnode);
	}

	if (ddf_fun_unbind(isa->fctl) != EOK) {
		fibril_mutex_unlock(&isa->mutex);
		ddf_msg(LVL_ERROR, "Failed unbinding control function.");
		return EXDEV;
	}

	fibril_mutex_unlock(&isa->mutex);

	return EOK;
}

static errno_t isa_fun_online(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "isa_fun_online()");
	return ddf_fun_online(fun);
}

static errno_t isa_fun_offline(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "isa_fun_offline()");
	return ddf_fun_offline(fun);
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS ISA bus driver\n");
	ddf_log_init(NAME);
	return ddf_driver_main(&isa_driver);
}

/**
 * @}
 */
