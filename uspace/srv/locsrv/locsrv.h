/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup locsrv
 * @{
 */
/** @file HelenOS location service.
 */

#ifndef LOCSRV_H_
#define LOCSRV_H_

#include <ipc/loc.h>
#include <async.h>
#include <fibril_synch.h>
#include <stddef.h>

/** Representation of server (supplier).
 *
 * Each server supplies a set of services.
 *
 */
typedef struct {
	/** Link to servers_list */
	link_t servers;

	/** List of services supplied by this server */
	list_t services;

	/** Session asociated with this server */
	async_sess_t *sess;

	/** Server name */
	char *name;

	/** Fibril mutex for list of services owned by this server */
	fibril_mutex_t services_mutex;
} loc_server_t;

/** Info about registered namespaces
 *
 */
typedef struct {
	/** Link to namespaces_list */
	link_t namespaces;

	/** Unique namespace identifier */
	service_id_t id;

	/** Namespace name */
	char *name;

	/** Reference count */
	size_t refcnt;
} loc_namespace_t;

/** Info about registered service
 *
 */
typedef struct {
	/** Link to global list of services (services_list) */
	link_t services;

	/** Link to server list of services (loc_server_t.services) */
	link_t server_services;

	/** Link to list of services in category (category_t.services) */
	link_t cat_services;

	/** List of category memberships (svc_categ_t) */
	list_t cat_memb;

	/** Unique service identifier */
	service_id_t id;

	/** Service namespace */
	loc_namespace_t *namespace;

	/** Service name */
	char *name;

	/** Supplier of this service */
	loc_server_t *server;
} loc_service_t;

extern fibril_mutex_t services_list_mutex;

extern service_id_t loc_create_id(void);
extern void loc_category_change_event(void);

#endif

/** @}
 */
