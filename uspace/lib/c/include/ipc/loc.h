/*
 * Copyright (c) 2007 Josef Cejka
 * Copyright (c) 2011 Jiri Svoboda
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
