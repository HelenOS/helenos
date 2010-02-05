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
 * @defgroup devman Device manager.
 * @brief HelenOS device manager.
 * @{
 */

/** @file
 */
 
#include <ipc/services.h>
#include <ipc/ns.h>
#include <async.h>
#include <stdio.h>
#include <errno.h>
#include <bool.h>
#include <fibril_synch.h>
#include <stdlib.h>
#include <string.h>
//#include <ipc/devman.h>

#define NAME          "devman"

#define MAX_ID_LEN 256


struct driver;
struct match_id;
struct match_id_list;
struct node;
struct dev_tree;


typedef struct driver driver_t;
typedef struct match_id match_id_t;
typedef struct match_id_list match_id_list_t;
typedef struct node node_t;
typedef struct dev_tree dev_tree_t;



LIST_INITIALIZE(drivers_list);

/** Represents device tree.
 */
struct dev_tree {
	node_t *root_node;
}



/** Ids of device models used for device-to-driver matching.
 */
struct match_id {
	/** Pointers to next and previous ids.
	 */
	link_t ids;
	/** Id of device model.
	 */
	char id[MAX_ID_LEN];
	/** Relevancy of device-to-driver match. 
	 * The higher is the product of scores specified for the device by the bus driver and by the leaf driver, 
	 * the more suitable is the leaf driver for handling the device. 
	 */
	unsigned int score;
};

/** List of ids for matching devices to drivers sorted 
 * according to match scores in descending order.
 */
struct match_id_list {
	link_t ids;
};

/** Representation of device driver.
 */
struct driver {	
	/** Pointers to previous and next drivers in a linked list */
	link_t drivers;
	/** Specifies whether the driver has been started.*/
	bool running;
	/** Phone asociated with this driver */
	ipcarg_t phone;
	/** Name of the device driver */
	char *name;
	/** Path to the driver's binary */ 
	const char *path;
	/** Fibril mutex for list of devices owned by this driver */
	fibril_mutex_t devices_mutex;	
	/** List of device ids for device-to-driver matching.*/
	match_id_list_t match_ids;
};

/** Representation of a node in the device tree.*/
struct node {
	/** The node of the parent device. */
	node_t *parent;
	/** Pointers to previous and next child devices in the linked list of parent device's node.*/
	link_t sibling;
	
	
	/** List of device ids for device-to-driver matching.*/
	match_id_list_t match_ids;
};

/** Initialize device manager internal structures.
 */
static bool devman_init() 
{
	// TODO:
	// initialize list of available drivers
	
	return true;
}

/**
 *
 */
int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS Device Manager\n");
	
	if (!devman_init()) {
		printf(NAME ": Error while initializing service\n");
		return -1;
	}
	
	/*// Set a handler of incomming connections 
	async_set_client_connection(devman_connection);
	
	// Register device manager at naming service 
	ipcarg_t phonead;
	if (ipc_connect_to_me(PHONE_NS, SERVICE_DEVMAN, 0, 0, &phonead) != 0) 
		return -1;
	
	printf(NAME ": Accepting connections\n");
	async_manager();*/
	
	// Never reached 
	return 0;
}