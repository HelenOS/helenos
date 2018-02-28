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

/** @addtogroup devman
 * @{
 */

#ifndef LIBC_IPC_DEVMAN_H_
#define LIBC_IPC_DEVMAN_H_

#include <ipc/common.h>
#include <adt/list.h>
#include <mem.h>
#include <stdlib.h>

#define DEVMAN_NAME_MAXLEN  256

typedef sysarg_t devman_handle_t;

typedef enum {
	/** Driver has not been started. */
	DRIVER_NOT_STARTED = 0,

	/**
	 * Driver has been started, but has not registered as running and ready
	 * to receive requests.
	 */
	DRIVER_STARTING,

	/** Driver is running and prepared to serve incomming requests. */
	DRIVER_RUNNING
} driver_state_t;

typedef enum {
	/** Invalid value for debugging purposes */
	fun_invalid = 0,
	/** Function to which child devices attach */
	fun_inner,
	/** Fuction exported to external clients (leaf function) */
	fun_exposed
} fun_type_t;

/** Ids of device models used for device-to-driver matching.
 */
typedef struct match_id {
	/** Pointers to next and previous ids.
	 */
	link_t link;
	/** Id of device model.
	 */
	char *id;
	/** Relevancy of device-to-driver match.
	 * The higher is the product of scores specified for the device by the bus driver and by the leaf driver,
	 * the more suitable is the leaf driver for handling the device.
	 */
	unsigned int score;
} match_id_t;

/** List of ids for matching devices to drivers sorted
 * according to match scores in descending order.
 */
typedef struct match_id_list {
	list_t ids;
} match_id_list_t;

static inline match_id_t *create_match_id(void)
{
	match_id_t *id = malloc(sizeof(match_id_t));
	memset(id, 0, sizeof(match_id_t));
	return id;
}

static inline void delete_match_id(match_id_t *id)
{
	if (id) {
		if (NULL != id->id) {
			free(id->id);
		}
		free(id);
	}
}

static inline void add_match_id(match_id_list_t *ids, match_id_t *id)
{
	match_id_t *mid = NULL;
	link_t *link = ids->ids.head.next;

	while (link != &ids->ids.head) {
		mid = list_get_instance(link, match_id_t,link);
		if (mid->score < id->score) {
			break;
		}
		link = link->next;
	}

	list_insert_before(&id->link, link);
}

static inline void init_match_ids(match_id_list_t *id_list)
{
	list_initialize(&id_list->ids);
}

static inline void clean_match_ids(match_id_list_t *ids)
{
	link_t *link = NULL;
	match_id_t *id;

	while (!list_empty(&ids->ids)) {
		link = list_first(&ids->ids);
		list_remove(link);
		id = list_get_instance(link, match_id_t, link);
		delete_match_id(id);
	}
}

typedef enum {
	DEVMAN_DRIVER_REGISTER = IPC_FIRST_USER_METHOD,
	DEVMAN_ADD_FUNCTION,
	DEVMAN_ADD_MATCH_ID,
	DEVMAN_ADD_DEVICE_TO_CATEGORY,
	DEVMAN_DRV_FUN_ONLINE,
	DEVMAN_DRV_FUN_OFFLINE,
	DEVMAN_REMOVE_FUNCTION
} driver_to_devman_t;

typedef enum {
	DRIVER_DEV_ADD = IPC_FIRST_USER_METHOD,
	DRIVER_DEV_REMOVE,
	DRIVER_DEV_GONE,
	DRIVER_FUN_ONLINE,
	DRIVER_FUN_OFFLINE,
	DRIVER_STOP
} devman_to_driver_t;

typedef enum {
	DEVMAN_DEVICE_GET_HANDLE = IPC_FIRST_USER_METHOD,
	DEVMAN_DEV_GET_FUNCTIONS,
	DEVMAN_DEV_GET_PARENT,
	DEVMAN_FUN_GET_CHILD,
	DEVMAN_FUN_GET_MATCH_ID,
	DEVMAN_FUN_GET_NAME,
	DEVMAN_FUN_GET_DRIVER_NAME,
	DEVMAN_FUN_ONLINE,
	DEVMAN_FUN_OFFLINE,
	DEVMAN_FUN_GET_PATH,
	DEVMAN_FUN_SID_TO_HANDLE,
	DEVMAN_GET_DRIVERS,
	DEVMAN_DRIVER_GET_DEVICES,
	DEVMAN_DRIVER_GET_HANDLE,
	DEVMAN_DRIVER_GET_MATCH_ID,
	DEVMAN_DRIVER_GET_NAME,
	DEVMAN_DRIVER_GET_STATE,
	DEVMAN_DRIVER_LOAD,
	DEVMAN_DRIVER_UNLOAD
} client_to_devman_t;

#endif

/** @}
 */
