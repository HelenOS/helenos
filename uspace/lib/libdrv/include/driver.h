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

/** @addtogroup libdrv
 * @{
 */
/** @file
 */
#ifndef LIBDRV_DRIVER_H_
#define LIBDRV_DRIVER_H_


#include <adt/list.h>
#include <ipc/devman.h>

typedef struct device {
	device_handle_t handle;
	ipcarg_t parent_phone;	
	const char *name;
	match_id_list_t match_ids;
	void *driver_data;
	
	// TODO add more items
	
	link_t link;
} device_t;

typedef struct driver_ops {	
	bool (*add_device)(device_t *dev);
	// TODO add other generic driver operations
} driver_ops_t;

typedef struct driver {
	const char *name;
	driver_ops_t *driver_ops;
} driver_t;

int driver_main(driver_t *drv);

static inline device_t * create_device()
{
	device_t *dev = malloc(sizeof(device_t));
	if (NULL != dev) {
		memset(dev, 0, sizeof(device_t));
	}	
	list_initialize(&dev->match_ids.ids);
	return dev;
}

static inline void delete_device(device_t *dev) {
	clean_match_ids(&dev->match_ids);
	if (NULL != dev->name) {
		free(dev->name);
	}
	free(dev);
}

bool child_device_register(device_t *child, device_t *parent);


#endif

/**
 * @}
 */