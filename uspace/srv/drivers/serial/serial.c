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
 * @defgroup serial Serial port driver.
 * @brief HelenOS serial port driver.
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
#include <ddi.h>
#include <libarch/ddi.h>

#include <driver.h>
#include <resource.h>

#include <devman.h>
#include <ipc/devman.h>
#include <device/hw_res.h>

#include "cyclic_buffer.h"

#define NAME "serial"

#define REG_COUNT 7

typedef struct serial_dev_data {
	bool client_connected;
	int irq;
	uint32_t io_addr;
	ioport8_t *port;
	cyclic_buffer_t input_buffer;
	fibril_mutex_t mutex;		
} serial_dev_data_t;

static serial_dev_data_t * create_serial_dev_data()
{
	serial_dev_data_t *data = (serial_dev_data_t *)malloc(sizeof(serial_dev_data_t));
	if (NULL != data) {
		memset(data, 0, sizeof(serial_dev_data_t));
		fibril_mutex_initialize(&data->mutex);
	}
	return data;	
}

static void delete_serial_dev_data(serial_dev_data_t *data) 
{
	if (NULL != data) {
		free(data);
	}
}

static device_class_t serial_dev_class;

static int serial_add_device(device_t *dev);

/** The serial port device driver's standard operations.
 */
static driver_ops_t serial_ops = {
	.add_device = &serial_add_device
};

/** The serial port device driver structure. 
 */
static driver_t serial_driver = {
	.name = NAME,
	.driver_ops = &serial_ops
};

static void serial_dev_cleanup(device_t *dev)
{
	if (NULL != dev->driver_data) {
		delete_serial_dev_data((serial_dev_data_t*)dev->driver_data);	
		dev->driver_data = NULL;
	}
	
	if (dev->parent_phone > 0) {
		ipc_hangup(dev->parent_phone);
		dev->parent_phone = 0;
	}	
}

static bool serial_pio_enable(device_t *dev)
{
	printf(NAME ": serial_pio_enable %s\n", dev->name);
	
	serial_dev_data_t *data = (serial_dev_data_t *)dev->driver_data;
	
	// Gain control over port's registers.
	if (pio_enable((void *)data->io_addr, REG_COUNT, (void **)(&data->port))) {  
		printf(NAME ": error - cannot gain the port %lx for device %s.\n", data->io_addr, dev->name);
		return false;
	}
	
	return true;
}

static bool serial_dev_probe(device_t *dev)
{
	printf(NAME ": serial_dev_probe %s\n", dev->name);
	
	serial_dev_data_t *data = (serial_dev_data_t *)dev->driver_data;
	ioport8_t *port_addr = data->port;	
	bool res = true;
	uint8_t olddata;
	
	olddata = pio_read_8(port_addr + 4);
	
	pio_write_8(port_addr + 4, 0x10);
	if (pio_read_8(port_addr + 6) & 0xf0) {
		res = false;
	}
	
	pio_write_8(port_addr + 4, 0x1f);
	if ((pio_read_8(port_addr + 6) & 0xf0) != 0xf0) {
		res = false;
	}
	
	pio_write_8(port_addr + 4, olddata);
	
	if (!res) {
		printf(NAME ": device %s is not present.\n", dev->name);
	}
	
	return res;	
}

static int serial_dev_initialize(device_t *dev)
{
	printf(NAME ": serial_dev_initialize %s\n", dev->name);
	
	int ret = EOK;
	hw_resource_list_t hw_resources;
	memset(&hw_resources, 0, sizeof(hw_resource_list_t));
	
	// allocate driver data for the device
	serial_dev_data_t *data = create_serial_dev_data();	
	if (NULL == data) {
		return ENOMEM;
	}
	dev->driver_data = data;
	
	// connect to the parent's driver
	dev->parent_phone = devman_parent_device_connect(dev->handle,  IPC_FLAG_BLOCKING);
	if (dev->parent_phone <= 0) {
		printf(NAME ": failed to connect to the parent driver of the device %s.\n", dev->name);
		ret = EPARTY;
		goto failed;
	}
	
	// get hw resources
	
	if (!get_hw_resources(dev->parent_phone, &hw_resources)) {
		printf(NAME ": failed to get hw resources for the device %s.\n", dev->name);
		ret = EPARTY;
		goto failed;
	}	
	
	size_t i;
	hw_resource_t *res;
	bool irq = false;
	bool ioport = false;
	
	for (i = 0; i < hw_resources.count; i++) {
		res = &hw_resources.resources[i];
		switch (res->type) {
		case INTERRUPT:
			data->irq = res->res.interrupt.irq;
			irq = true;
			printf(NAME ": the %s device was asigned irq = 0x%x.\n", dev->name, data->irq);
			break;
		case IO_RANGE:
			data->io_addr = res->res.io_range.address;
			if (res->res.io_range.size < REG_COUNT) {
				printf(NAME ": i/o range assigned to the device %s is too small.\n", dev->name);
				ret = EPARTY;
				goto failed;
			}
			ioport = true;
			printf(NAME ": the %s device was asigned i/o address = 0x%x.\n", dev->name, data->io_addr);
			break;			
		}
	}
	
	if (!irq || !ioport) {
		printf(NAME ": missing hw resource(s) for the device %s.\n", dev->name);
		ret = EPARTY;
		goto failed;
	}		
	
	clean_hw_resource_list(&hw_resources);
	return ret;
	
failed:
	serial_dev_cleanup(dev);	
	clean_hw_resource_list(&hw_resources);	
	return ret;	
}

static int serial_interrupt_enable(device_t *dev)
{
	serial_dev_data_t *data = (serial_dev_data_t *)dev->driver_data;
	
	// enable interrupt globally	
	enable_interrupt(data->irq);
	
	// enable interrupt on the serial port
	pio_write_8(data->port + 1 , 0x01);   // Interrupt when data received
	pio_write_8(data->port + 4, 0x0B);
	
	return EOK;
}

static void serial_initialize_port(device_t *dev)
{
	serial_dev_data_t *data = (serial_dev_data_t *)dev->driver_data;
	ioport8_t *port = data->port;
	
	pio_write_8(port + 1, 0x00);    // Disable all interrupts
	pio_write_8(port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
	pio_write_8(port + 0, 0x60);    // Set divisor to 96 (lo byte) 1200 baud
	pio_write_8(port + 1, 0x00);    //                   (hi byte)
	pio_write_8(port + 3, 0x07);    // 8 bits, no parity, two stop bits
	pio_write_8(port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
	pio_write_8(port + 4, 0x0B);    // RTS/DSR set (Request to Send and Data Terminal Ready lines enabled), 
									// Aux Output2 set - needed for interrupts	
}

static int serial_add_device(device_t *dev) 
{
	printf(NAME ": serial_add_device %s (handle = %d)\n", dev->name, dev->handle);
	
	int res = serial_dev_initialize(dev);
	if (EOK != res) {
		return res;
	}
	
	if (!serial_pio_enable(dev)) {
		serial_dev_cleanup(dev);
		return EADDRNOTAVAIL;
	}	
	
	// find out whether the device is present
	if (!serial_dev_probe(dev)) {
		serial_dev_cleanup(dev);
		return ENOENT;
	}	
	
	// serial port initialization (baud rate etc.)
	serial_initialize_port(dev);
	
	// TODO register interrupt handler
	
	// enable interrupt
	if (0 != (res = serial_interrupt_enable(dev))) {
		serial_dev_cleanup(dev);
		return res;
	}		
	
	return EOK;
}

static void serial_init() 
{
	// TODO
	serial_dev_class.id = 0;
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS serial port driver\n");	
	serial_init();
	return driver_main(&serial_driver);
}

/**
 * @}
 */