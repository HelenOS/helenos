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
 * @defgroup ns8250 Serial port driver.
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
#include <str.h>
#include <ctype.h>
#include <macros.h>
#include <malloc.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ddi.h>
#include <libarch/ddi.h>

#include <driver.h>
#include <char.h>
#include <resource.h>

#include <devman.h>
#include <ipc/devman.h>
#include <device/hw_res.h>
#include <ipc/serial_ctl.h>

#include "cyclic_buffer.h"

#define NAME "ns8250"

#define REG_COUNT 7
#define MAX_BAUD_RATE 115200
#define DLAB_MASK (1 << 7)

/** The number of bits of one data unit send by the serial port.*/
typedef enum {
	WORD_LENGTH_5,
	WORD_LENGTH_6,
	WORD_LENGTH_7,
	WORD_LENGTH_8	
} word_length_t;

/** The number of stop bits used by the serial port. */
typedef enum {
	/** Use one stop bit. */
	ONE_STOP_BIT,
	/** 1.5 stop bits for word length 5, 2 stop bits otherwise. */
	TWO_STOP_BITS	
} stop_bit_t;

/** The driver data for the serial port devices.
 */
typedef struct ns8250_dev_data {
	/** Is there any client conntected to the device? */
	bool client_connected;
	/** The irq assigned to this device. */
	int irq;
	/** The base i/o address of the devices registers. */
	uint32_t io_addr;
	/** The i/o port used to access the serial ports registers. */
	ioport8_t *port;
	/** The buffer for incomming data.*/
	cyclic_buffer_t input_buffer;
	/** The fibril mutex for synchronizing the access to the device.*/
	fibril_mutex_t mutex;		
} ns8250_dev_data_t;

/** Create driver data for a device.
 * 
 * @return the driver data. 
 */
static ns8250_dev_data_t * create_ns8250_dev_data()
{
	ns8250_dev_data_t *data = (ns8250_dev_data_t *)malloc(sizeof(ns8250_dev_data_t));
	if (NULL != data) {
		memset(data, 0, sizeof(ns8250_dev_data_t));
		fibril_mutex_initialize(&data->mutex);
	}
	return data;	
}

/** Delete driver data.
 * 
 * @param data the driver data structure.
 */
static void delete_ns8250_dev_data(ns8250_dev_data_t *data) 
{
	if (NULL != data) {
		free(data);
	}
}

/** Find out if there is some incomming data available on the serial port.
 * 
 * @param port the base address of the serial port device's ports.
 * @return true if there are data waiting to be read, false otherwise. 
 */
static bool ns8250_received(ioport8_t *port) 
{
   return (pio_read_8(port + 5) & 1) != 0;
}

/** Read one byte from the serial port.
 * 
 * @param port the base address of the serial port device's ports.
 * @return the data read. 
 */
static uint8_t ns8250_read_8(ioport8_t *port) 
{
	return pio_read_8(port);
}

/** Find out wheter it is possible to send data.
 * 
 * @param port the base address of the serial port device's ports.
 */
static bool is_transmit_empty(ioport8_t *port) 
{
   return (pio_read_8(port + 5) & 0x20) != 0;
}

/** Write one character on the serial port.
 * 
 * @param port the base address of the serial port device's ports.
 * @param c the character to be written to the serial port device.
 */
static void ns8250_write_8(ioport8_t *port, uint8_t c) 
{
	while (!is_transmit_empty(port)) 
		;
	
	pio_write_8(port, c);
}

/** Read data from the serial port device.
 * 
 * @param dev the serial port device.
 * @param buf the ouput buffer for read data.
 * @param count the number of bytes to be read.
 * 
 * @return the number of bytes actually read on success, negative error number otherwise.
 */
static int ns8250_read(device_t *dev, char *buf, size_t count) 
{
	// printf(NAME ": ns8250_read %s\n", dev->name);
	
	int ret = EOK;
	
	ns8250_dev_data_t *data = (ns8250_dev_data_t *)dev->driver_data;
	fibril_mutex_lock(&data->mutex);
	
	while (!buf_is_empty(&data->input_buffer) && (size_t)ret < count) {
		buf[ret] = (char)buf_pop_front(&data->input_buffer);
		ret++;
	}
	
	fibril_mutex_unlock(&data->mutex);
	
	return ret;
}

/** Write a character to the serial port.
 * 
 * @param data the serial port device's driver data.
 * @param c the character to be written. 
 */
static inline void ns8250_putchar(ns8250_dev_data_t *data, uint8_t c)
{	
	fibril_mutex_lock(&data->mutex);
	ns8250_write_8(data->port, c);	
	fibril_mutex_unlock(&data->mutex);
}

/** Write data to the serial port.
 * 
 * @param dev the serial port device.
 * @param buf the data to be written.
 * @param count the number of bytes to be written.
 * 
 * @return 0 on success.
 */
static int ns8250_write(device_t *dev, char *buf, size_t count) 
{
	ns8250_dev_data_t *data = (ns8250_dev_data_t *)dev->driver_data;
	
	size_t idx;
	for (idx = 0; idx < count; idx++) {
		ns8250_putchar(data, (uint8_t)buf[idx]);
	}
	
	return 0;
}

static device_class_t ns8250_dev_class;

/** The character interface's callbacks. 
 */
static char_iface_t ns8250_char_iface = {
	.read = &ns8250_read,
	.write = &ns8250_write
};

static int ns8250_add_device(device_t *dev);

/** The serial port device driver's standard operations.
 */
static driver_ops_t ns8250_ops = {
	.add_device = &ns8250_add_device
};

/** The serial port device driver structure. 
 */
static driver_t ns8250_driver = {
	.name = NAME,
	.driver_ops = &ns8250_ops
};

/** Clean up the serial port device structure.
 * 
 * @param dev the device structure.
 */
static void ns8250_dev_cleanup(device_t *dev)
{
	if (NULL != dev->driver_data) {
		delete_ns8250_dev_data((ns8250_dev_data_t*)dev->driver_data);	
		dev->driver_data = NULL;
	}
	
	if (dev->parent_phone > 0) {
		ipc_hangup(dev->parent_phone);
		dev->parent_phone = 0;
	}	
}

/** Enable the i/o ports of the device.
 * 
 * @param the serial port device.
 * @return true on success, false otherwise.
 */
static bool ns8250_pio_enable(device_t *dev)
{
	printf(NAME ": ns8250_pio_enable %s\n", dev->name);
	
	ns8250_dev_data_t *data = (ns8250_dev_data_t *)dev->driver_data;
	
	// Gain control over port's registers.
	if (pio_enable((void *)data->io_addr, REG_COUNT, (void **)(&data->port))) {  
		printf(NAME ": error - cannot gain the port %lx for device %s.\n", data->io_addr, dev->name);
		return false;
	}
	
	return true;
}

/** Probe the serial port device for its presence.
 * 
 * @param dev the serial port device.
 * @return true if the device is present, false otherwise.
 */
static bool ns8250_dev_probe(device_t *dev)
{
	printf(NAME ": ns8250_dev_probe %s\n", dev->name);
	
	ns8250_dev_data_t *data = (ns8250_dev_data_t *)dev->driver_data;
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

/** Initialize serial port device.
 * 
 * @param dev the serial port device.
 * @return 0 on success, negative error number otherwise.
 */
static int ns8250_dev_initialize(device_t *dev)
{
	printf(NAME ": ns8250_dev_initialize %s\n", dev->name);
	
	int ret = EOK;
	hw_resource_list_t hw_resources;
	memset(&hw_resources, 0, sizeof(hw_resource_list_t));
	
	// allocate driver data for the device
	ns8250_dev_data_t *data = create_ns8250_dev_data();	
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
		default:
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
	ns8250_dev_cleanup(dev);	
	clean_hw_resource_list(&hw_resources);	
	return ret;	
}

/** Enable interrupts on the serial port device.
 * 
 * Interrupt when data is received.
 * 
 * @param port the base address of the serial port device's ports. 
 */
static inline void ns8250_port_interrupts_enable(ioport8_t *port)
{	
	pio_write_8(port + 1 , 0x01);   // Interrupt when data received
	pio_write_8(port + 4, 0x0B);	
}

/** Disable interrupts on the serial port device.
 * 
 * @param port the base address of the serial port device's ports. 
 */
static inline void ns8250_port_interrupts_disable(ioport8_t *port)
{
	pio_write_8(port + 1, 0x00);    // Disable all interrupts
}

/** Enable interrupts for the serial port device.
 * 
 * @param dev the device.
 * @return 0 on success, negative error number otherwise.
 */
static int ns8250_interrupt_enable(device_t *dev)
{
	ns8250_dev_data_t *data = (ns8250_dev_data_t *)dev->driver_data;
	
	int res;
	// enable interrupt globally	
	if (EOK != (res = interrupt_enable(data->irq))) {
		return res;
	}
	
	// enable interrupt on the serial port
	ns8250_port_interrupts_enable(data->port);
	
	return EOK;
}

/** Set Divisor Latch Access Bit.
 * 
 * When the Divisor Latch Access Bit is set, 
 * it is possible to set baud rate of the serial port device.
 * 
 * @param port the base address of the serial port device's ports. 
 */
static inline void enable_dlab(ioport8_t *port)
{
	uint8_t val = pio_read_8(port + 3);
	pio_write_8(port + 3, val | DLAB_MASK);	
}

/** Clear Divisor Latch Access Bit.
 * 
 * @param port the base address of the serial port device's ports. 
 */
static inline void clear_dlab(ioport8_t *port)
{
	uint8_t val = pio_read_8(port + 3);
	pio_write_8(port + 3, val & (~DLAB_MASK));
}

/** Set baud rate of the serial communication on the serial device.
 * 
 * @param port the base address of the serial port device's ports. 
 * @param baud_rate the baud rate to be used by the device.
 * 
 * @return 0 on success, negative error number otherwise (EINVAL if the specified baud_rate is not valid). 
 */
static int ns8250_port_set_baud_rate(ioport8_t *port, unsigned int baud_rate)
{
	uint16_t divisor;
	uint8_t div_low, div_high;
	
	if (50 > baud_rate || 0 != MAX_BAUD_RATE % baud_rate) {
		printf(NAME ": error - somebody tried to set invalid baud rate %d\n", baud_rate);
		return EINVAL; 
	}
	
	divisor = MAX_BAUD_RATE / baud_rate;
	div_low = (uint8_t)divisor;
	div_high = (uint8_t)(divisor >> 8);	
	
	// enable DLAB to be able to access baud rate divisor
	enable_dlab(port);    
	
	// set divisor low byte
	pio_write_8(port + 0, div_low); 
	// set divisor high byte
	pio_write_8(port + 1, div_high);    
	
	clear_dlab(port);	
	
	return EOK;		
}

/** Get baud rate used by the serial port device.
 * 
 * @param port the base address of the serial port device's ports.
 * @param baud_rate the ouput parameter to which the baud rate is stored.
 */
static unsigned int ns8250_port_get_baud_rate(ioport8_t *port)
{
	uint16_t divisor;
	uint8_t div_low, div_high;
	
	// enable DLAB to be able to access baud rate divisor
	enable_dlab(port);
	
	// get divisor low byte
	div_low = pio_read_8(port + 0);
	// get divisor high byte
	div_high = pio_read_8(port + 1);   
	
	clear_dlab(port);
	
	divisor = (div_high << 8) | div_low;
	return MAX_BAUD_RATE / divisor;
}

/** Get the parameters of the serial communication set on the serial port device.
 * 
 * @param parity the parity used.
 * @param word_length the length of one data unit in bits.
 * @param stop_bits the number of stop bits used (one or two). 
 */
static void ns8250_port_get_com_props(
	ioport8_t *port, unsigned int *parity, unsigned int *word_length, unsigned int *stop_bits)
{
	uint8_t val;
	
	val = pio_read_8(port + 3);
	
	*parity = ((val >> 3) & 7);
	
	switch (val & 3) {
		case WORD_LENGTH_5:
			*word_length = 5;
			break;
		case WORD_LENGTH_6:
			*word_length = 6;
			break;
		case WORD_LENGTH_7:
			*word_length = 7;
			break;
		case WORD_LENGTH_8:
			*word_length = 8;
			break;
	}
	
	if ((val >> 2) & 1) {
		*stop_bits = 2;
	} else {
		*stop_bits = 1;
	}	
}

/** Set the parameters of the serial communication on the serial port device.
 * 
 * @param parity the parity to be used.
 * @param word_length the length of one data unit in bits.
 * @param stop_bits the number of stop bits used (one or two). 
 * 
 * @return 0 on success, EINVAL if some of the specified values is invalid.
 */
static int ns8250_port_set_com_props(
	ioport8_t *port, unsigned int parity, unsigned int word_length, unsigned int stop_bits)
{
	uint8_t val;
	
	switch (word_length) {
		case 5:
			val = WORD_LENGTH_5;
			break;
		case 6:
			val = WORD_LENGTH_6;
			break;
		case 7:
			val = WORD_LENGTH_7;
			break;
		case 8:
			val = WORD_LENGTH_8;
			break;
		default:
			return EINVAL;
	}
	
	switch (stop_bits) {
		case 1:
			val |= ONE_STOP_BIT << 2;
			break;
		case 2:
			val |= TWO_STOP_BITS << 2;
			break;
		default:
			return EINVAL;
	}
	
	switch (parity) {
		case SERIAL_NO_PARITY:
		case SERIAL_ODD_PARITY:
		case SERIAL_EVEN_PARITY:
		case SERIAL_MARK_PARITY:
		case SERIAL_SPACE_PARITY:	
			val |= parity << 3;
			break;
		default:
			return EINVAL;
	}
	
	pio_write_8(port + 3, val);	
	
	return EOK;
}

/** Initialize the serial port device.
 * 
 * Set the default parameters of the serial communication.
 * 
 * @param dev the serial port device.
 */
static void ns8250_initialize_port(device_t *dev)
{
	ns8250_dev_data_t *data = (ns8250_dev_data_t *)dev->driver_data;
	ioport8_t *port = data->port;
	
	// disable interrupts
	ns8250_port_interrupts_disable(port); 
    // set baud rate
	ns8250_port_set_baud_rate(port, 38400);
	// 8 bits, no parity, two stop bits
	ns8250_port_set_com_props(port, SERIAL_NO_PARITY, 8, 2);  
	// Enable FIFO, clear them, with 14-byte threshold
	pio_write_8(port + 2, 0xC7);  	
	// RTS/DSR set (Request to Send and Data Terminal Ready lines enabled), 
	// Aux Output2 set - needed for interrupts
	pio_write_8(port + 4, 0x0B);    										
}

/** Read the data from the serial port device and store them to the input buffer.
 * 
 * @param dev the serial port device.
 */
static void ns8250_read_from_device(device_t *dev)
{
	ns8250_dev_data_t *data = (ns8250_dev_data_t *)dev->driver_data;
	ioport8_t *port = data->port;
	bool cont = true;
	
	while (cont) {	
		fibril_mutex_lock(&data->mutex);
		
		cont = ns8250_received(port);
		if (cont) {
			uint8_t val = ns8250_read_8(port);
			// printf(NAME ": character %c read from %s.\n", val, dev->name);			
			
			if (data->client_connected) {
				if (!buf_push_back(&(data->input_buffer), val)) {
					printf(NAME ": buffer overflow on %s.\n", dev->name);
				} else {
					printf(NAME ": the character %c saved to the buffer of %s.\n", val, dev->name);
				}
			} else {
				// printf(NAME ": no client is connected to %s, discarding the character which was read.\n", dev->name);
			}			
		}
		
		fibril_mutex_unlock(&data->mutex);	
		
		fibril_yield();		
	}	
}

/** The interrupt handler.
 * 
 * The serial port is initialized to interrupt when some data come, 
 * so the interrupt is handled by reading the incomming data.
 * 
 * @param dev the serial port device. 
 */
static inline void ns8250_interrupt_handler(device_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	ns8250_read_from_device(dev);
}

/** Register the interrupt handler for the device.
 * 
 * @param dev the serial port device.
 */
static inline int ns8250_register_interrupt_handler(device_t *dev)
{
	ns8250_dev_data_t *data = (ns8250_dev_data_t *)dev->driver_data;
	
	return register_interrupt_handler(dev, data->irq, ns8250_interrupt_handler, NULL);	
}

/** Unregister the interrupt handler for the device.
 * 
 * @param dev the serial port device.
 */
static inline int ns8250_unregister_interrupt_handler(device_t *dev)
{
	ns8250_dev_data_t *data = (ns8250_dev_data_t *)dev->driver_data;
	
	return unregister_interrupt_handler(dev, data->irq);	
}

/** The add_device callback method of the serial port driver.
 * 
 * Probe and initialize the newly added device.
 * 
 * @param dev the serial port device. 
 */
static int ns8250_add_device(device_t *dev) 
{
	printf(NAME ": ns8250_add_device %s (handle = %d)\n", dev->name, dev->handle);
	
	int res = ns8250_dev_initialize(dev);
	if (EOK != res) {
		return res;
	}
	
	if (!ns8250_pio_enable(dev)) {
		ns8250_dev_cleanup(dev);
		return EADDRNOTAVAIL;
	}	
	
	// find out whether the device is present
	if (!ns8250_dev_probe(dev)) {
		ns8250_dev_cleanup(dev);
		return ENOENT;
	}	
	
	// serial port initialization (baud rate etc.)
	ns8250_initialize_port(dev);
	
	// register interrupt handler
	if (EOK != ns8250_register_interrupt_handler(dev)) {
		printf(NAME ": failed to register interrupt handler.\n");
		ns8250_dev_cleanup(dev);
		return res;
	}
	
	// enable interrupt
	if (EOK != (res = ns8250_interrupt_enable(dev))) {
		printf(NAME ": failed to enable the interrupt. Error code = %d.\n", res);
		ns8250_dev_cleanup(dev);
		ns8250_unregister_interrupt_handler(dev);
		return res;
	}	
	
	dev->class = &ns8250_dev_class;
	
	add_device_to_class(dev, "serial");
	
	printf(NAME ": the %s device has been successfully initialized.\n", dev->name);
	
	return EOK;
}

/** Open the device.
 * 
 * This is a callback function called when a client tries to connect to the device.
 * 
 * @param dev the device.
 */
static int ns8250_open(device_t *dev)
{
	ns8250_dev_data_t *data = (ns8250_dev_data_t *)dev->driver_data;
	int res;
	
	fibril_mutex_lock(&data->mutex);	
	
	if (data->client_connected) {
		res = ELIMIT;
	} else {
		res = EOK;
		data->client_connected = true;
	}
	
	fibril_mutex_unlock(&data->mutex);

	return res;
}

/** Close the device.
 * 
 *  This is a callback function called when a client tries to disconnect from the device.
 * 
 * @param dev the device. 
 */
static void ns8250_close(device_t *dev)
{
	ns8250_dev_data_t *data = (ns8250_dev_data_t *)dev->driver_data;
	
	fibril_mutex_lock(&data->mutex);
	
	assert(data->client_connected);	
	
	data->client_connected = false;
	buf_clear(&data->input_buffer);
	
	fibril_mutex_unlock(&data->mutex);	 
}

/** Get parameters of the serial communication which are set to the specified device.
 * 
 * @param the serial port device.
 * @param baud_rate the baud rate used by the device.
 * @param the type of parity used by the device.
 * @param word_length the size of one data unit in bits.
 * @param stop_bits the number of stop bits used.
 */
static void ns8250_get_props(device_t *dev, unsigned int *baud_rate, 
	unsigned int *parity, unsigned int *word_length, unsigned int* stop_bits)
{	
	ns8250_dev_data_t *data = (ns8250_dev_data_t *)dev->driver_data;
	ioport8_t *port = data->port;
	
	fibril_mutex_lock(&data->mutex);	
	ns8250_port_interrupts_disable(port);    // Disable all interrupts
	*baud_rate = ns8250_port_get_baud_rate(port);
	ns8250_port_get_com_props(port, parity, word_length, stop_bits);	
	ns8250_port_interrupts_enable(port);
	fibril_mutex_unlock(&data->mutex);	
	
	printf(NAME ": ns8250_get_props: baud rate %d, parity 0x%x, word length %d, stop bits %d\n", 
		*baud_rate, *parity, *word_length, * stop_bits);
}

/** Set parameters of the serial communication to the specified  serial port device.
 * 
 * @param the serial port device.
 * @param baud_rate the baud rate to be used by the device.
 * @param the type of parity to be used by the device.
 * @param word_length the size of one data unit in bits.
 * @param stop_bits the number of stop bits to be used.
 */
static int ns8250_set_props(device_t *dev, unsigned int baud_rate, 
	unsigned int parity, unsigned int word_length, unsigned int stop_bits)
{
	printf(NAME ": ns8250_set_props: baud rate %d, parity 0x%x, word length %d, stop bits %d\n", 
		baud_rate, parity, word_length, stop_bits);
	
	ns8250_dev_data_t *data = (ns8250_dev_data_t *)dev->driver_data;
	ioport8_t *port = data->port;
	int ret;
	
	fibril_mutex_lock(&data->mutex);	
	ns8250_port_interrupts_disable(port);    // Disable all interrupts
	ret = ns8250_port_set_baud_rate(port, baud_rate);
	if (EOK == ret) {
		ret = ns8250_port_set_com_props(port, parity, word_length, stop_bits);
	}
	ns8250_port_interrupts_enable(port);
	fibril_mutex_unlock(&data->mutex);	
	
	return ret;		
}


/** Default handler for client requests which are not handled by the standard interfaces.
 * 
 * Configure the parameters of the serial communication.
 */
static void ns8250_default_handler(device_t *dev, ipc_callid_t callid, ipc_call_t *call)
{
	ipcarg_t method = IPC_GET_METHOD(*call);
	int ret;
	unsigned int baud_rate, parity, word_length, stop_bits;	
	
	switch(method) {
		case SERIAL_GET_COM_PROPS:
			ns8250_get_props(dev, &baud_rate, &parity, &word_length, &stop_bits);
			ipc_answer_4(callid, EOK, baud_rate, parity, word_length, stop_bits);
			break;
		
		case SERIAL_SET_COM_PROPS:
 			baud_rate = IPC_GET_ARG1(*call);
			parity = IPC_GET_ARG2(*call);
			word_length = IPC_GET_ARG3(*call);
			stop_bits = IPC_GET_ARG4(*call);
			ret = ns8250_set_props(dev, baud_rate, parity, word_length, stop_bits);
			ipc_answer_0(callid, ret);
			break;
			
		default:
			ipc_answer_0(callid, ENOTSUP);		
	}	
}

/** Initialize the serial port driver.
 * 
 * Initialize class structures with callback methods for handling 
 * client requests to the serial port devices.
 */
static void ns8250_init() 
{
	// TODO
	ns8250_dev_class.id = 0;
	ns8250_dev_class.open = &ns8250_open;
	ns8250_dev_class.close = &ns8250_close;	
	
	ns8250_dev_class.interfaces[CHAR_DEV_IFACE] = &ns8250_char_iface;
	ns8250_dev_class.default_handler = &ns8250_default_handler;
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS serial port driver\n");	
	ns8250_init();
	return driver_main(&ns8250_driver);
}

/**
 * @}
 */