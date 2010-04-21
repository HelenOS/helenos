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

#include <driver.h>
#include <resource.h>

#include <devman.h>
#include <ipc/devman.h>
#include <device/hw_res.h>

#define NAME "serial"

typedef struct serial_dev_data {
		
} serial_dev_data_t;


static device_class_t serial_dev_class;

static bool serial_add_device(device_t *dev);

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

static bool serial_add_device(device_t *dev) 
{
	printf(NAME ": serial_add_device, device handle = %d\n", dev->handle);
	
	// TODO
	
	return true;
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