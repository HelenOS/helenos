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
#include <bool.h>
#include <fibril_synch.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>
#include <ctype.h>
#include <macros.h>
#include <malloc.h>
#include <dirent.h>
#include <fcntl.h>
#include <ipc/irc.h>
#include <ipc/services.h>
#include <sysinfo.h>
#include <ns.h>
#include <sys/stat.h>
#include <ipc/irc.h>
#include <ipc/services.h>
#include <sysinfo.h>
#include <ns.h>

#include <ddf/driver.h>
#include <ddf/log.h>
#include <ops/hw_res.h>

#include <device/hw_res.h>

#include "i8237.h"

#define NAME "isa"
#define CHILD_FUN_CONF_PATH "/drv/isa/isa.dev"

/** Obtain soft-state from device node */
#define ISA_BUS(dev) ((isa_bus_t *) ((dev)->driver_data))

/** Obtain soft-state from function node */
#define ISA_FUN(fun) ((isa_fun_t *) ((fun)->driver_data))

#define ISA_MAX_HW_RES 5

typedef struct {
	fibril_mutex_t mutex;
	ddf_dev_t *dev;
	ddf_fun_t *fctl;
	list_t functions;
} isa_bus_t;

typedef struct isa_fun {
	fibril_mutex_t mutex;
	ddf_fun_t *fnode;
	hw_resource_list_t hw_resources;
	link_t bus_link;
} isa_fun_t;

static hw_resource_list_t *isa_get_fun_resources(ddf_fun_t *fnode)
{
	isa_fun_t *fun = ISA_FUN(fnode);
	assert(fun != NULL);

	return &fun->hw_resources;
}

static bool isa_enable_fun_interrupt(ddf_fun_t *fnode)
{
	/* This is an old ugly way, copied from pci driver */
	assert(fnode);
	isa_fun_t *isa_fun = fnode->driver_data;

	sysarg_t apic;
	sysarg_t i8259;

	async_sess_t *irc_sess = NULL;

	if (((sysinfo_get_value("apic", &apic) == EOK) && (apic))
	    || ((sysinfo_get_value("i8259", &i8259) == EOK) && (i8259))) {
		irc_sess = service_connect_blocking(EXCHANGE_SERIALIZE,
		    SERVICE_IRC, 0, 0);
	}

	if (!irc_sess)
		return false;

	assert(isa_fun);
	const hw_resource_list_t *res = &isa_fun->hw_resources;
	assert(res);
	for (size_t i = 0; i < res->count; ++i) {
		if (res->resources[i].type == INTERRUPT) {
			const int irq = res->resources[i].res.interrupt.irq;

			async_exch_t *exch = async_exchange_begin(irc_sess);
			const int rc =
			    async_req_1_0(exch, IRC_ENABLE_INTERRUPT, irq);
			async_exchange_end(exch);

			if (rc != EOK) {
				async_hangup(irc_sess);
				return false;
			}
		}
	}

	async_hangup(irc_sess);
	return true;
}

static int isa_dma_channel_fun_setup(ddf_fun_t *fnode,
    unsigned int channel, uint32_t pa, uint16_t size, uint8_t mode)
{
	assert(fnode);
	isa_fun_t *isa_fun = fnode->driver_data;
	const hw_resource_list_t *res = &isa_fun->hw_resources;
	assert(res);
	
	const unsigned int ch = channel;
	for (size_t i = 0; i < res->count; ++i) {
		if (((res->resources[i].type == DMA_CHANNEL_16) &&
		    (res->resources[i].res.dma_channel.dma16 == ch)) ||
		    ((res->resources[i].type == DMA_CHANNEL_8) &&
		    (res->resources[i].res.dma_channel.dma8 == ch))) {
			return dma_setup_channel(channel, pa, size, mode);
		}
	}
	
	return EINVAL;
}

static hw_res_ops_t isa_fun_hw_res_ops = {
	.get_resource_list = isa_get_fun_resources,
	.enable_interrupt = isa_enable_fun_interrupt,
	.dma_channel_setup = isa_dma_channel_fun_setup,
};

static ddf_dev_ops_t isa_fun_ops;

static int isa_dev_add(ddf_dev_t *dev);
static int isa_dev_remove(ddf_dev_t *dev);
static int isa_fun_online(ddf_fun_t *fun);
static int isa_fun_offline(ddf_fun_t *fun);

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
	fun->fnode = fnode;
	return fun;
}

static char *fun_conf_read(const char *conf_path)
{
	bool suc = false;
	char *buf = NULL;
	bool opened = false;
	int fd;
	size_t len = 0;

	fd = open(conf_path, O_RDONLY);
	if (fd < 0) {
		ddf_msg(LVL_ERROR, "Unable to open %s", conf_path);
		goto cleanup;
	}

	opened = true;

	len = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
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

	if (0 >= read(fd, buf, len)) {
		ddf_msg(LVL_ERROR, "Unable to read file '%s'.", conf_path);
		goto cleanup;
	}

	buf[len] = 0;

	suc = true;

cleanup:
	if (!suc && buf != NULL) {
		free(buf);
		buf = NULL;
	}

	if (opened)
		close(fd);

	return buf;
}

static char *str_get_line(char *str, char **next)
{
	char *line = str;

	if (str == NULL) {
		*next = NULL;
		return NULL;
	}

	while (*str != '\0' && *str != '\n') {
		str++;
	}

	if (*str != '\0') {
		*next = str + 1;
	} else {
		*next = NULL;
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
	strtok(line, ":");

	/* Allocate output buffer. */
	size_t size = str_size(line) + 1;
	char *name = malloc(size);

	if (name != NULL) {
		/* Copy the result to the output buffer. */
		str_cpy(name, size, line);
	}

	return name;
}

static inline char *skip_spaces(char *line)
{
	/* Skip leading spaces. */
	while (*line != '\0' && isspace(*line))
		line++;

	return line;
}

static void isa_fun_set_irq(isa_fun_t *fun, int irq)
{
	size_t count = fun->hw_resources.count;
	hw_resource_t *resources = fun->hw_resources.resources;

	if (count < ISA_MAX_HW_RES) {
		resources[count].type = INTERRUPT;
		resources[count].res.interrupt.irq = irq;

		fun->hw_resources.count++;

		ddf_msg(LVL_NOTE, "Added irq 0x%x to function %s", irq,
		    fun->fnode->name);
	}
}

static void isa_fun_set_dma(isa_fun_t *fun, int dma)
{
	size_t count = fun->hw_resources.count;
	hw_resource_t *resources = fun->hw_resources.resources;
	
	if (count < ISA_MAX_HW_RES) {
		if ((dma > 0) && (dma < 4)) {
			resources[count].type = DMA_CHANNEL_8;
			resources[count].res.dma_channel.dma8 = dma;
			
			fun->hw_resources.count++;
			ddf_msg(LVL_NOTE, "Added dma 0x%x to function %s", dma,
			    fun->fnode->name);
			
			return;
		}

		if ((dma > 4) && (dma < 8)) {
			resources[count].type = DMA_CHANNEL_16;
			resources[count].res.dma_channel.dma16 = dma;
			
			fun->hw_resources.count++;
			ddf_msg(LVL_NOTE, "Added dma 0x%x to function %s", dma,
			    fun->fnode->name);
			
			return;
		}
		
		ddf_msg(LVL_WARN, "Skipped dma 0x%x for function %s", dma,
		    fun->fnode->name);
	}
}

static void isa_fun_set_io_range(isa_fun_t *fun, size_t addr, size_t len)
{
	size_t count = fun->hw_resources.count;
	hw_resource_t *resources = fun->hw_resources.resources;

	if (count < ISA_MAX_HW_RES) {
		resources[count].type = IO_RANGE;
		resources[count].res.io_range.address = addr;
		resources[count].res.io_range.size = len;
		resources[count].res.io_range.endianness = LITTLE_ENDIAN;

		fun->hw_resources.count++;

		ddf_msg(LVL_NOTE, "Added io range (addr=0x%x, size=0x%x) to "
		    "function %s", (unsigned int) addr, (unsigned int) len,
		    fun->fnode->name);
	}
}

static void fun_parse_irq(isa_fun_t *fun, char *val)
{
	int irq = 0;
	char *end = NULL;

	val = skip_spaces(val);
	irq = (int) strtol(val, &end, 10);

	if (val != end)
		isa_fun_set_irq(fun, irq);
}

static void fun_parse_dma(isa_fun_t *fun, char *val)
{
	unsigned int dma = 0;
	char *end = NULL;
	
	val = skip_spaces(val);
	dma = (unsigned int) strtol(val, &end, 10);
	
	if (val != end)
		isa_fun_set_dma(fun, dma);
}

static void fun_parse_io_range(isa_fun_t *fun, char *val)
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

	isa_fun_set_io_range(fun, addr, len);
}

static void get_match_id(char **id, char *val)
{
	char *end = val;

	while (!isspace(*end))
		end++;

	size_t size = end - val + 1;
	*id = (char *)malloc(size);
	str_cpy(*id, size, val);
}

static void fun_parse_match_id(isa_fun_t *fun, char *val)
{
	char *id = NULL;
	int score = 0;
	char *end = NULL;
	int rc;

	val = skip_spaces(val);

	score = (int)strtol(val, &end, 10);
	if (val == end) {
		ddf_msg(LVL_ERROR, "Cannot read match score for function "
		    "%s.", fun->fnode->name);
		return;
	}

	val = skip_spaces(end);
	get_match_id(&id, val);
	if (id == NULL) {
		ddf_msg(LVL_ERROR, "Cannot read match ID for function %s.",
		    fun->fnode->name);
		return;
	}

	ddf_msg(LVL_DEBUG, "Adding match id '%s' with score %d to "
	    "function %s", id, score, fun->fnode->name);

	rc = ddf_fun_add_match_id(fun->fnode, id, score);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed adding match ID: %s",
		    str_error(rc));
	}

	free(id);
}

static bool prop_parse(isa_fun_t *fun, char *line, const char *prop,
    void (*read_fn)(isa_fun_t *, char *))
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

static void fun_prop_parse(isa_fun_t *fun, char *line)
{
	/* Skip leading spaces. */
	line = skip_spaces(line);

	if (!prop_parse(fun, line, "io_range", &fun_parse_io_range) &&
	    !prop_parse(fun, line, "irq", &fun_parse_irq) &&
	    !prop_parse(fun, line, "dma", &fun_parse_dma) &&
	    !prop_parse(fun, line, "match", &fun_parse_match_id)) {

		ddf_msg(LVL_ERROR, "Undefined device property at line '%s'",
		    line);
	}
}

static void fun_hw_res_alloc(isa_fun_t *fun)
{
	fun->hw_resources.resources =
	    (hw_resource_t *) malloc(sizeof(hw_resource_t) * ISA_MAX_HW_RES);
}

static void fun_hw_res_free(isa_fun_t *fun)
{
	free(fun->hw_resources.resources);
	fun->hw_resources.resources = NULL;
}

static char *isa_fun_read_info(char *fun_conf, isa_bus_t *isa)
{
	char *line;
	char *fun_name = NULL;

	/* Skip empty lines. */
	while (true) {
		line = str_get_line(fun_conf, &fun_conf);

		if (line == NULL) {
			/* no more lines */
			return NULL;
		}

		if (!line_empty(line))
			break;
	}

	/* Get device name. */
	fun_name = get_device_name(line);
	if (fun_name == NULL)
		return NULL;

	isa_fun_t *fun = isa_fun_create(isa, fun_name);
	free(fun_name);
	if (fun == NULL) {
		return NULL;
	}

	/* Allocate buffer for the list of hardware resources of the device. */
	fun_hw_res_alloc(fun);

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
	fun->fnode->ops = &isa_fun_ops;

	ddf_msg(LVL_DEBUG, "Binding function %s.", fun->fnode->name);

	/* XXX Handle error */
	(void) ddf_fun_bind(fun->fnode);

	list_append(&fun->bus_link, &isa->functions);

	return fun_conf;
}

static void fun_conf_parse(char *conf, isa_bus_t *isa)
{
	while (conf != NULL && *conf != '\0') {
		conf = isa_fun_read_info(conf, isa);
	}
}

static void isa_functions_add(isa_bus_t *isa)
{
	char *fun_conf;

	fun_conf = fun_conf_read(CHILD_FUN_CONF_PATH);
	if (fun_conf != NULL) {
		fun_conf_parse(fun_conf, isa);
		free(fun_conf);
	}
}

static int isa_dev_add(ddf_dev_t *dev)
{
	isa_bus_t *isa;

	ddf_msg(LVL_DEBUG, "isa_dev_add, device handle = %d",
	    (int) dev->handle);

	isa = ddf_dev_data_alloc(dev, sizeof(isa_bus_t));
	if (isa == NULL)
		return ENOMEM;

	fibril_mutex_initialize(&isa->mutex);
	isa->dev = dev;
	list_initialize(&isa->functions);

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

static int isa_dev_remove(ddf_dev_t *dev)
{
	isa_bus_t *isa = ISA_BUS(dev);
	int rc;

	fibril_mutex_lock(&isa->mutex);

	while (!list_empty(&isa->functions)) {
		isa_fun_t *fun = list_get_instance(list_first(&isa->functions),
		    isa_fun_t, bus_link);

		rc = ddf_fun_offline(fun->fnode);
		if (rc != EOK) {
			fibril_mutex_unlock(&isa->mutex);
			ddf_msg(LVL_ERROR, "Failed offlining %s", fun->fnode->name);
			return rc;
		}

		rc = ddf_fun_unbind(fun->fnode);
		if (rc != EOK) {
			fibril_mutex_unlock(&isa->mutex);
			ddf_msg(LVL_ERROR, "Failed unbinding %s", fun->fnode->name);
			return rc;
		}

		list_remove(&fun->bus_link);

		fun_hw_res_free(fun);
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

static int isa_fun_online(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "isa_fun_online()");
	return ddf_fun_online(fun);
}

static int isa_fun_offline(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "isa_fun_offline()");
	return ddf_fun_offline(fun);
}


static void isa_init()
{
	ddf_log_init(NAME, LVL_ERROR);
	isa_fun_ops.interfaces[HW_RES_DEV_IFACE] = &isa_fun_hw_res_ops;
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS ISA bus driver\n");
	isa_init();
	return ddf_driver_main(&isa_driver);
}

/**
 * @}
 */
