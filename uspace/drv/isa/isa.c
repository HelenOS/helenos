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

#define ISA_MAX_HW_RES 4

typedef struct isa_fun_data {
	hw_resource_list_t hw_resources;
} isa_fun_data_t;

static hw_resource_list_t *isa_get_fun_resources(function_t *fun)
{
	isa_fun_data_t *fun_data;

	fun_data = (isa_fun_data_t *)fun->driver_data;
	if (fun_data == NULL)
		return NULL;

	return &fun_data->hw_resources;
}

static bool isa_enable_fun_interrupt(function_t *fun)
{
	// TODO

	return false;
}

static hw_res_ops_t isa_fun_hw_res_ops = {
	&isa_get_fun_resources,
	&isa_enable_fun_interrupt
};

static device_ops_t isa_fun_dev_ops;

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


static isa_fun_data_t *create_isa_fun_data() 
{
	isa_fun_data_t *data;

	data = (isa_fun_data_t *) malloc(sizeof(isa_fun_data_t));
	if (data != NULL)
		memset(data, 0, sizeof(isa_fun_data_t));

	return data;
}

static function_t *create_isa_fun()
{
	function_t *fun = create_function();
	if (fun == NULL)
		return NULL;

	isa_fun_data_t *data = create_isa_fun_data();
	if (data == NULL) {
		delete_function(fun);
		return NULL;
	}

	fun->driver_data = data;
	return fun;
}

static char *read_fun_conf(const char *conf_path)
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
		printf(NAME ": read_fun_conf error: configuration file '%s' "
		    "is empty.\n", conf_path);
		goto cleanup;
	}

	buf = malloc(len + 1);
	if (buf == NULL) {
		printf(NAME ": read_fun_conf error: memory allocation failed.\n");
		goto cleanup;
	}

	if (0 >= read(fd, buf, len)) {
		printf(NAME ": read_fun_conf error: unable to read file '%s'.\n",
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

static void isa_fun_set_irq(function_t *fun, int irq)
{
	isa_fun_data_t *data = (isa_fun_data_t *)fun->driver_data;

	size_t count = data->hw_resources.count;
	hw_resource_t *resources = data->hw_resources.resources;

	if (count < ISA_MAX_HW_RES) {
		resources[count].type = INTERRUPT;
		resources[count].res.interrupt.irq = irq;

		data->hw_resources.count++;

		printf(NAME ": added irq 0x%x to function %s\n", irq, fun->name);
	}
}

static void isa_fun_set_io_range(function_t *fun, size_t addr, size_t len)
{
	isa_fun_data_t *data = (isa_fun_data_t *)fun->driver_data;

	size_t count = data->hw_resources.count;
	hw_resource_t *resources = data->hw_resources.resources;

	if (count < ISA_MAX_HW_RES) {
		resources[count].type = IO_RANGE;
		resources[count].res.io_range.address = addr;
		resources[count].res.io_range.size = len;
		resources[count].res.io_range.endianness = LITTLE_ENDIAN;

		data->hw_resources.count++;

		printf(NAME ": added io range (addr=0x%x, size=0x%x) to "
		    "function %s\n", (unsigned int) addr, (unsigned int) len,
		    fun->name);
	}
}

static void get_dev_irq(function_t *fun, char *val)
{
	int irq = 0;
	char *end = NULL;

	val = skip_spaces(val);
	irq = (int)strtol(val, &end, 0x10);

	if (val != end)
		isa_fun_set_irq(fun, irq);
}

static void get_dev_io_range(function_t *fun, char *val)
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

static void get_fun_match_id(function_t *fun, char *val)
{
	char *id = NULL;
	int score = 0;
	char *end = NULL;

	val = skip_spaces(val);	

	score = (int)strtol(val, &end, 10);
	if (val == end) {
		printf(NAME " : error - could not read match score for "
		    "function %s.\n", fun->name);
		return;
	}

	match_id_t *match_id = create_match_id();
	if (match_id == NULL) {
		printf(NAME " : failed to allocate match id for function %s.\n",
		    fun->name);
		return;
	}

	val = skip_spaces(end);	
	get_match_id(&id, val);
	if (id == NULL) {
		printf(NAME " : error - could not read match id for "
		    "function %s.\n", fun->name);
		delete_match_id(match_id);
		return;
	}

	match_id->id = id;
	match_id->score = score;

	printf(NAME ": adding match id '%s' with score %d to function %s\n", id,
	    score, fun->name);
	add_match_id(&fun->match_ids, match_id);
}

static bool read_fun_prop(function_t *fun, char *line, const char *prop,
    void (*read_fn)(function_t *, char *))
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

static void get_fun_prop(function_t *fun, char *line)
{
	/* Skip leading spaces. */
	line = skip_spaces(line);

	if (!read_fun_prop(fun, line, "io_range", &get_dev_io_range) &&
	    !read_fun_prop(fun, line, "irq", &get_dev_irq) &&
	    !read_fun_prop(fun, line, "match", &get_fun_match_id))
	{
	    printf(NAME " error undefined device property at line '%s'\n",
		line);
	}
}

static void child_alloc_hw_res(function_t *fun)
{
	isa_fun_data_t *data = (isa_fun_data_t *)fun->driver_data;
	data->hw_resources.resources = 
	    (hw_resource_t *)malloc(sizeof(hw_resource_t) * ISA_MAX_HW_RES);
}

static char *read_isa_fun_info(char *fun_conf, device_t *dev)
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

	function_t *fun = create_isa_fun();
	if (fun == NULL) {
		free(fun_name);
		return NULL;
	}

	fun->name = fun_name;
	fun->ftype = fun_inner;

	/* Allocate buffer for the list of hardware resources of the device. */
	child_alloc_hw_res(fun);

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
		get_fun_prop(fun, line);

		//printf(NAME ": next line ='%s'\n", fun_conf);
		//printf(NAME ": current line ='%s'\n", line);
	}

	/* Set device operations to the device. */
	fun->ops = &isa_fun_dev_ops;

	printf(NAME ": register_function(fun, dev); function is %s.\n",
	    fun->name);
	register_function(fun, dev);

	return fun_conf;
}

static void parse_fun_conf(char *conf, device_t *dev)
{
	while (conf != NULL && *conf != '\0') {
		conf = read_isa_fun_info(conf, dev);
	}
}

static void add_legacy_children(device_t *dev)
{
	char *fun_conf;

	fun_conf = read_fun_conf(CHILD_FUN_CONF_PATH);
	if (fun_conf != NULL) {
		parse_fun_conf(fun_conf, dev);
		free(fun_conf);
	}
}

static int isa_add_device(device_t *dev)
{
	printf(NAME ": isa_add_device, device handle = %d\n",
	    (int) dev->handle);

	/* Make the bus device more visible. Does not do anything. */
	printf(NAME ": adding a 'ctl' function\n");

	function_t *ctl = create_function();
	ctl->ftype = fun_exposed;
	ctl->name = "ctl";
	register_function(ctl, dev);

	/* Add child devices. */
	add_legacy_children(dev);
	printf(NAME ": finished the enumeration of legacy devices\n");

	return EOK;
}

static void isa_init() 
{
	isa_fun_dev_ops.interfaces[HW_RES_DEV_IFACE] = &isa_fun_hw_res_ops;
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
 
