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
#include <string.h>
#include <ctype.h>
#include <macros.h>
#include <malloc.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <driver.h>
#include <resource.h>

#include <devman.h>
#include <ipc/devman.h>
#include <device/hw_res.h>

#define NAME "isa"
#define CHILD_DEV_CONF_PATH "/srv/drivers/isa/isa.dev"

#define ISA_MAX_HW_RES 4

typedef struct isa_child_data {
	hw_resource_list_t hw_resources;	
} isa_child_data_t;

static hw_resource_list_t * isa_get_child_resources(device_t *dev)
{
	isa_child_data_t *dev_data = (isa_child_data_t *)dev->driver_data;
	if (NULL == dev_data) {
		return NULL;
	}
	return &dev_data->hw_resources;
}

static bool isa_enable_child_interrupt(device_t *dev) 
{
	// TODO
	
	return false;
}

static resource_iface_t isa_child_res_iface = {
	&isa_get_child_resources,
	&isa_enable_child_interrupt	
};

static device_class_t isa_child_class;

static bool isa_add_device(device_t *dev);

/** The isa device driver's standard operations.
 */
static driver_ops_t isa_ops = {
	.add_device = &isa_add_device
};

/** The isa device driver structure. 
 */
static driver_t isa_driver = {
	.name = NAME,
	.driver_ops = &isa_ops
};


static isa_child_data_t * create_isa_child_data() 
{
	 isa_child_data_t *data = (isa_child_data_t *) malloc(sizeof(isa_child_data_t));
	 if (NULL != data) {
		 memset(data, 0, sizeof(isa_child_data_t));
	 }
	 return data;	
}

static device_t * create_isa_child_dev()
{
	device_t *dev = create_device();
	if (NULL == dev) {
		return NULL;
	}
	isa_child_data_t *data = create_isa_child_data();
	if (NULL == data) {
		delete_device(dev);
		return NULL;
	}
	
	dev->driver_data = data;
	return dev;
}

static char * read_dev_conf(const char *conf_path)
{
	printf(NAME ": read_dev_conf conf_path = %s.\n", conf_path);
		
	bool suc = false;	
	char *buf = NULL;
	bool opened = false;
	int fd;		
	off_t len = 0;
	
	fd = open(conf_path, O_RDONLY);
	if (fd < 0) {
		printf(NAME ": unable to open %s\n", conf_path);
		goto cleanup;
	} 
	opened = true;	
	
	len = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);	
	if (len == 0) {
		printf(NAME ": read_dev_conf error: configuration file '%s' is empty.\n", conf_path);
		goto cleanup;		
	}
	
	buf = malloc(len + 1);
	if (buf == NULL) {
		printf(NAME ": read_dev_conf error: memory allocation failed.\n");
		goto cleanup;
	}	
	
	if (0 >= read(fd, buf, len)) {
		printf(NAME ": read_dev_conf error: unable to read file '%s'.\n", conf_path);
		goto cleanup;
	}
	buf[len] = 0;
	
	suc = true;
	
cleanup:
	
	if (!suc && NULL != buf) {
		free(buf);	
		buf = NULL;
	}
	
	if(opened) {
		close(fd);	
	}
	
	return buf;	
}

static char * str_get_line(char *str, char **next) {
	char *line = str;
	while (0 != *str && '\n' != *str) {
		str++;
	} 
	
	if (0 != *str) {
		*next = str + 1;
	} else {
		*next = NULL;
	}
	
	*str = 0;	
	
	return line;
}


static bool line_empty(const char *line)
{
	while (NULL != line && 0 != *line) {
		if(!isspace(*line)) {
			return false;
		}		
		line++;		
	}	
	return true;	
}

static char * get_device_name(char *line) {
	// skip leading spaces
	while (0 != *line && isspace(*line)) {
		line++;
	}
	
	// get the name part of the rest of the line
	strtok(line, ":");	
	
	// alloc output buffer
	size_t size = str_size(line) + 1;
	char *name = malloc(size);
	
	if (NULL != name) {
		// copy the result to the output buffer
		str_cpy(name, size, line);
	}

	return name;
}

static inline char * skip_spaces(char *line) 
{
	// skip leading spaces
	while (0 != *line && isspace(*line)) {
		line++;
	}
	return line;	
}


static void isa_child_set_irq(device_t *dev, int irq)
{
	isa_child_data_t *data = (isa_child_data_t *)dev->driver_data;
	
	size_t count = data->hw_resources.count;
	hw_resource_t *resources = data->hw_resources.resources;
	
	if (count < ISA_MAX_HW_RES) {
		resources[count].type = INTERRUPT;
		resources[count].res.interrupt.irq = irq;
		
		data->hw_resources.count++;
	}	
}

static void isa_child_set_io_range(device_t *dev, size_t addr, size_t len)
{
	isa_child_data_t *data = (isa_child_data_t *)dev->driver_data;
	
	size_t count = data->hw_resources.count;
	hw_resource_t *resources = data->hw_resources.resources;
	
	if (count < ISA_MAX_HW_RES) {
		resources[count].type = IO_RANGE;
		resources[count].res.io_range.address = addr;
		resources[count].res.io_range.size = len;
		resources[count].res.io_range.endianness = LITTLE_ENDIAN;		
		data->hw_resources.count++;
	}	
}

static void get_dev_irq(device_t *dev, char *val)
{
	int irq = 0;
	char *end = NULL;
	
	val = skip_spaces(val);	
	irq = (int)strtol(val, &end, 0x10);
	
	if (val != end) {
		isa_child_set_irq(dev, irq);		
	}
}

static void get_dev_io_range(device_t *dev, char *val)
{
	size_t addr, len;
	char *end = NULL;
	
	val = skip_spaces(val);	
	addr = strtol(val, &end, 0x10);
	
	if (val == end) {
		return;
	}
	
	val = skip_spaces(end);	
	len = strtol(val, &end, 0x10);
	
	if (val == end) {
		return;
	}
	
	isa_child_set_io_range(dev, addr, len);
}

static void get_match_id(char **id, char *val)
{
	char *end = val;
	
	while (isspace(*end)) {
		end++;
	}	
	*end = 0;
	
	size_t size = str_size(val) + 1;
	*id = (char *)malloc(size);
	str_cpy(*id, size, val);	
}

static void get_dev_match_id(device_t *dev, char *val)
{	
	char *id = NULL;
	int score = 0;
	char *end = NULL;
	
	val = skip_spaces(val);	
	
	score = (int)strtol(val, &end, 0x10);
	if (val == end) {
		return;
	}
	
	match_id_t *match_id = create_match_id();
	if (NULL == match_id) {
		return;
	}
	
	val = skip_spaces(end);	
	get_match_id(&id, val);
	if (NULL == id) {
		delete_match_id(match_id);
		return;
	}
	
	match_id->id = id;
	match_id->score = score;
	
	add_match_id(&dev->match_ids, match_id);
}

static void get_dev_prop(device_t *dev, char *line)
{
	// skip leading spaces
	line = skip_spaces(line);
	
	// value of the property
	char *val;
	
	if (NULL != (val = strtok(line, "io_range"))) {		
		get_dev_io_range(dev, val);
	} else if (NULL != (val = strtok(line, "irq"))) {
		get_dev_irq(dev, val);		
	} else if (NULL != (val = strtok(line, "match"))) {
		get_dev_match_id(dev, val);
	} 	
}

static void child_alloc_hw_res(device_t *dev) 
{
	isa_child_data_t *data = (isa_child_data_t *)dev->driver_data;
	data->hw_resources.resources = 
		(hw_resource_t *)malloc(sizeof(hw_resource_t) * ISA_MAX_HW_RES);
	
}

static char * read_isa_dev_info(char *dev_conf, device_t *parent)
{
	char *line;
	char *dev_name = NULL;
	
	// skip empty lines
	while (true) {	
		line = str_get_line(dev_conf, &dev_conf);
		
		if (NULL == line) {
			// no more lines
			return NULL;
		}
		
		if (!line_empty(line)) {
			break;
		}
	}
	
	// get device name
	dev_name = get_device_name(line);
	if (NULL == dev_name) {
		return NULL;
	}
	
	device_t *dev = create_isa_child_dev();
	if (NULL == dev) {
		free(dev_name);
		return NULL;
	}
	dev->name = dev_name;
	
	// allocate buffer for the list of hardware resources of the device
	child_alloc_hw_res(dev);
	
	// get properties of the device (match ids, irq and io range)
	while (true) {		
		line = str_get_line(dev_conf, &dev_conf);
		
		if (line_empty(line)) {
			// no more device properties
			break;
		}
		
		// get the device's property from the configuration line and store it in the device structure
		get_dev_prop(dev, line);
	}
	
	// set a class (including the corresponding set of interfaces) to the device
	dev->class = &isa_child_class;
	
	child_device_register(dev, parent);
	
	return dev_conf;	
}

static char * parse_dev_conf(char *conf, device_t *parent)
{
	while (NULL != conf) {
		conf = read_isa_dev_info(conf, parent);
	}	
}

static void add_legacy_children(device_t *parent)
{
	char *dev_conf = read_dev_conf(CHILD_DEV_CONF_PATH); 
	if (NULL != dev_conf) {
		parse_dev_conf(dev_conf, parent);	
		free(dev_conf);
	}
}

static bool isa_add_device(device_t *dev) 
{
	printf(NAME ": isa_add_device, device handle = %d\n", dev->handle);
	
	// add child devices	
	add_legacy_children(dev);
	
	return true;
}

static void isa_init() 
{
	isa_child_class.id = 0; // TODO
	isa_child_class.interfaces[HW_RES_DEV_IFACE] = &isa_child_res_iface;
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
 