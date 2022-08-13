/*
 * SPDX-FileCopyrightText: 2007 Josef Cejka
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */

#ifndef _LIBC_IPC_LOC_H_
#define _LIBC_IPC_LOC_H_

#include <ipc/common.h>

#define LOC_NAME_MAXLEN  255

typedef sysarg_t service_id_t;
typedef sysarg_t category_id_t;

typedef enum {
	LOC_OBJECT_NONE,
	LOC_OBJECT_NAMESPACE,
	LOC_OBJECT_SERVICE
} loc_object_type_t;

typedef enum {
	LOC_SERVER_REGISTER = IPC_FIRST_USER_METHOD,
	LOC_SERVER_UNREGISTER,
	LOC_SERVICE_ADD_TO_CAT,
	LOC_SERVICE_REGISTER,
	LOC_SERVICE_UNREGISTER,
	LOC_SERVICE_GET_ID,
	LOC_SERVICE_GET_NAME,
	LOC_SERVICE_GET_SERVER_NAME,
	LOC_NAMESPACE_GET_ID,
	LOC_CALLBACK_CREATE,
	LOC_CATEGORY_GET_ID,
	LOC_CATEGORY_GET_NAME,
	LOC_CATEGORY_GET_SVCS,
	LOC_ID_PROBE,
	LOC_NULL_CREATE,
	LOC_NULL_DESTROY,
	LOC_GET_NAMESPACE_COUNT,
	LOC_GET_SERVICE_COUNT,
	LOC_GET_CATEGORIES,
	LOC_GET_NAMESPACES,
	LOC_GET_SERVICES
} loc_request_t;

typedef enum {
	LOC_EVENT_CAT_CHANGE = IPC_FIRST_USER_METHOD
} loc_event_t;

typedef struct {
	service_id_t id;
	char name[LOC_NAME_MAXLEN + 1];
} loc_sdesc_t;

#endif

/** @}
 */
