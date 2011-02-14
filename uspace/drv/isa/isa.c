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
 * @defgroup isa ISA bus driver.
 * @brief HelenOS ISA bus driver.
 * @{
 */

/** @file
 */

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
#include <sys/stat.h>

#include <driver.h>
#include <ops/hw_res.h>

#include <devman.h>
#include <ipc/devman.h>
#include <device/hw_res.h>

#define NAME "isa"
#define CHILD_FUN_CONF_PATH "/drv/isa/isa.dev"

/** Obtain soft-state pointer from function node pointer */
#define ISA_FUN(fnode) ((isa_fun_t *) ((fnode)->driver_data))

#define ISA_MAX_HW_RES 4

typedef struct isa_fun {
	function_t *fnode;
	hw_resource_list_t hw_resources;
} isa_fun_t;

static hw_resource_list_t *isa_get_fun_resources(function_t *fnode)
{
	isa_fun_t *fun = ISA_FUN(fnode);
	assert(fun != NULL);

	return &fun->hw_resources;
}

static bool isa_enable_fun_interrupt(function_t *fnode)
{
	// TODO

	return false;
}

static hw_res_ops_t isa_fun_hw_res_ops = {
	&isa_get_fun_resources,
	&isa_enable_fun_interrupt
};

static device_ops_t isa_fun_ops;

static int isa_add_device(device_t *dev);

/** The isa device driver's standard operations */
static driver_ops_t isa_ops = {
	.add_device = &isa_add_device
};

/** The isa device driver structure. */
static driver_t isa_driver = {
	.name = NAME,
	.driver_ops = &isa_ops
};

static isa_fun_t *isa_fun_create(device_t *dev, const char *name)
{
	isa_fun_t *fun = calloc(1, sizeof(isa_fun_t));
	if (fun == NULL)
		return NULL;

	function_t *fnode = ddf_fun_create(dev, fun_inner, name);
	if (fnode == NULL) {
		free(fun);
		return NULL;
	}

	fun->fnode = fnode;
	fnode->driver_data = fun;
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
		printf(NAME ": unable to open %s\n", conf_path);
		goto cleanup;
	}

	opened = true;

	len = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);	
	if (len == 0) {
		printf(NAME ": fun_conf_read error: configuration file '%s' "
		    "is empty.\n", conf_path);
		goto cleanup;
	}

	buf = malloc(len + 1);
	if (buf == NULL) {
		printf(NAME ": fun_conf_read error: memory allocation failed.\n");
		goto cleanup;
	}

	if (0 >= read(fd, buf, len)) {
		printf(NAME ": fun_conf_read error: unable to read file '%s'.\n",
		    conf_path);
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

		printf(NAME ": added irq 0x%x to function %s\n", irq,
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

		printf(NAME ": added io range (addr=0x%x, size=0x%x) to "
		    "function %s\n", (unsigned int) addr, (unsigned int) len,
		    fun->fnode->name);
	}
}

static void fun_parse_irq(isa_fun_t *fun, char *val)
{
	int irq = 0;
	char *end = NULL;

	val = skip_spaces(val);
	irq = (int)strtol(val, &end, 0x10);

	if (val != end)
		isa_fun_set_irq(fun, irq);
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
		printf(NAME " : error - could not read match score for "
		    "function %s.\n", fun->fnode->name);
		return;
	}

	val = skip_spaces(end);
	get_match_id(&id, val);
	if (id == NULL) {
		printf(NAME " : error - could not read match id for "
		    "function %s.\n", fun->fnode->name);
		return;
	}

	printf(NAME ": adding match id '%s' with score %d to function %s\n", id,
	    score, fun->fnode->name);

	rc = ddf_fun_add_match_id(fun->fnode, id, score);
	if (rc != EOK)
		printf(NAME ": error adding match ID: %s\n", str_error(rc));
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
	    !prop_parse(fun, line, "match", &fun_parse_match_id))
	{
	    printf(NAME " error undefined device property at line '%s'\n",
		line);
	}
}

static void fun_hw_res_alloc(isa_fun_t *fun)
{
	fun->hw_resources.resources = 
	    (hw_resource_t *)malloc(sizeof(hw_resource_t) * ISA_MAX_HW_RES);
}

static char *isa_fun_read_info(char *fun_conf, device_t *dev)
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

	isa_fun_t *fun = isa_fun_create(dev, fun_name);
	if (fun == NULL) {
		free(fun_name);
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

	printf(NAME ": Binding function %s.\n", fun->fnode->name);

	/* XXX Handle error */
	(void) ddf_fun_bind(fun->fnode);

	return fun_conf;
}

static void fun_conf_parse(char *conf, device_t *dev)
{
	while (conf != NULL && *conf != '\0') {
		conf = isa_fun_read_info(conf, dev);
	}
}

static void isa_functions_add(device_t *dev)
{
	char *fun_conf;

	fun_conf = fun_conf_read(CHILD_FUN_CONF_PATH);
	if (fun_conf != NULL) {
		fun_conf_parse(fun_conf, dev);
		free(fun_conf);
	}
}

static int isa_add_device(device_t *dev)
{
	printf(NAME ": isa_add_device, device handle = %d\n",
	    (int) dev->handle);

	/* Make the bus device more visible. Does not do anything. */
	printf(NAME ": adding a 'ctl' function\n");

	function_t *ctl = ddf_fun_create(dev, fun_exposed, "ctl");
	if (ctl == NULL) {
		printf(NAME ": Error creating control function.\n");
		return EXDEV;
	}

	if (ddf_fun_bind(ctl) != EOK) {
		printf(NAME ": Error binding control function.\n");
		return EXDEV;
	}

	/* Add functions as specified in the configuration file. */
	isa_functions_add(dev);
	printf(NAME ": finished the enumeration of legacy functions\n");

	return EOK;
}

static void isa_init() 
{
	isa_fun_ops.interfaces[HW_RES_DEV_IFACE] = &isa_fun_hw_res_ops;
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS ISA bus driver\n");
	isa_init();
	return driver_main(&isa_driver);
}

/**
 * @}
 */
