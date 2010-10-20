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
 * @defgroup root Root device driver.
 * @brief HelenOS root device driver.
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

#include <driver.h>
#include <devman.h>
#include <ipc/devman.h>

#define NAME "root"

static int root_add_device(device_t *dev);

/** The root device driver's standard operations.
 */
static driver_ops_t root_ops = {
	.add_device = &root_add_device
};

/** The root device driver structure. 
 */
static driver_t root_driver = {
	.name = NAME,
	.driver_ops = &root_ops
};

/** Create the device which represents the root of HW device tree.
 * 
 * @param parent parent of the newly created device.
 * @return 0 on success, negative error number otherwise.
 */
static int add_platform_child(device_t *parent) {
	printf(NAME ": adding new child for platform device.\n");
	
	int res = EOK;
	device_t *platform = NULL;
	match_id_t *match_id = NULL;	
	
	// create new device
	if (NULL == (platform = create_device())) {
		res = ENOMEM;
		goto failure;
	}	
	
	platform->name = "hw";
	printf(NAME ": the new device's name is %s.\n", platform->name);
	
	// initialize match id list
	if (NULL == (match_id = create_match_id())) {
		res = ENOMEM;
		goto failure;
	}
	
	// TODO - replace this with some better solution (sysinfo ?)
	match_id->id = STRING(UARCH);
	match_id->score = 100;
	add_match_id(&platform->match_ids, match_id);	
	
	// register child  device
	res = child_device_register(platform, parent);
	if (EOK != res) {
		goto failure;
	}
	
	return res;
	
failure:
	if (NULL != match_id) {
		match_id->id = NULL;
	}
	
	if (NULL != platform) {
		platform->name = NULL;
		delete_device(platform);		
	}
	
	return res;	
}

/** Get the root device.
 * @param dev the device which is root of the whole device tree (both of HW and pseudo devices).
 */
static int root_add_device(device_t *dev) 
{
	printf(NAME ": root_add_device, device handle = %d\n", dev->handle);
	
	// register root device's children	
	int res = add_platform_child(dev);	
	if (EOK != res) {
		printf(NAME ": failed to add child device for platform.\n");
	}
	
	return res;
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS root device driver\n");	
	return driver_main(&root_driver);
}

/**
 * @}
 */
 