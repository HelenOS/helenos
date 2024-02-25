/*
 * Copyright (c) 2024 Jiri Svoboda
 * Copyright (c) 2007 Josef Cejka
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
 * @addtogroup locsrv
 * @{
 */

/** @file
 */

#include <ipc/services.h>
#include <ns.h>
#include <async.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <fibril_synch.h>
#include <macros.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>
#include <ipc/loc.h>
#include <assert.h>

#include "category.h"
#include "locsrv.h"

#define NAME          "loc"
#define NULL_SERVICES  256

/** Callback session */
typedef struct {
	link_t cb_sess_list;
	async_sess_t *sess;
} cb_sess_t;

LIST_INITIALIZE(services_list);
LIST_INITIALIZE(namespaces_list);
LIST_INITIALIZE(servers_list);

/*
 * Locking order:
 *  servers_list_mutex
 *  services_list_mutex
 *  (loc_server_t *)->services_mutex
 *  create_id_mutex
 */

FIBRIL_MUTEX_INITIALIZE(services_list_mutex);
static FIBRIL_CONDVAR_INITIALIZE(services_list_cv);
static FIBRIL_MUTEX_INITIALIZE(servers_list_mutex);
static FIBRIL_MUTEX_INITIALIZE(create_id_mutex);
static FIBRIL_MUTEX_INITIALIZE(null_services_mutex);

static service_id_t last_id = 0;
static loc_service_t *null_services[NULL_SERVICES];

/*
 * Dummy list for null services. This is necessary so that null services can
 * be used just as any other services, e.g. in loc_service_unregister_core().
 */
static LIST_INITIALIZE(dummy_null_services);

/** Service directory ogranized by categories (yellow pages) */
static categ_dir_t cdir;

static FIBRIL_MUTEX_INITIALIZE(callback_sess_mutex);
static LIST_INITIALIZE(callback_sess_list);

service_id_t loc_create_id(void)
{
	/*
	 * TODO: allow reusing old ids after their unregistration
	 * and implement some version of LRU algorithm, avoid overflow
	 */

	fibril_mutex_lock(&create_id_mutex);
	last_id++;
	fibril_mutex_unlock(&create_id_mutex);

	return last_id;
}

/** Convert fully qualified service name to namespace and service name.
 *
 * A fully qualified service name can be either a plain service name
 * (then the namespace is considered to be an empty string) or consist
 * of two components separated by a slash. No more than one slash
 * is allowed.
 *
 */
static bool loc_fqsn_split(const char *fqsn, char **ns_name, char **name)
{
	size_t cnt = 0;
	size_t slash_offset = 0;
	size_t slash_after = 0;

	size_t offset = 0;
	size_t offset_prev = 0;
	char32_t c;

	while ((c = str_decode(fqsn, &offset, STR_NO_LIMIT)) != 0) {
		if (c == '/') {
			cnt++;
			slash_offset = offset_prev;
			slash_after = offset;
		}
		offset_prev = offset;
	}

	/* More than one slash */
	if (cnt > 1)
		return false;

	/* No slash -> namespace is empty */
	if (cnt == 0) {
		*ns_name = str_dup("");
		if (*ns_name == NULL)
			return false;

		*name = str_dup(fqsn);
		if (*name == NULL) {
			free(*ns_name);
			return false;
		}

		if (str_cmp(*name, "") == 0) {
			free(*name);
			free(*ns_name);
			return false;
		}

		return true;
	}

	/* Exactly one slash */
	*ns_name = str_ndup(fqsn, slash_offset);
	if (*ns_name == NULL)
		return false;

	*name = str_dup(fqsn + slash_after);
	if (*name == NULL) {
		free(*ns_name);
		return false;
	}

	if (str_cmp(*name, "") == 0) {
		free(*name);
		free(*ns_name);
		return false;
	}

	return true;
}

/** Find namespace with given name. */
static loc_namespace_t *loc_namespace_find_name(const char *name)
{
	assert(fibril_mutex_is_locked(&services_list_mutex));

	list_foreach(namespaces_list, namespaces, loc_namespace_t, namespace) {
		if (str_cmp(namespace->name, name) == 0)
			return namespace;
	}

	return NULL;
}

/** Find namespace with given ID.
 *
 * @todo: use hash table
 *
 */
static loc_namespace_t *loc_namespace_find_id(service_id_t id)
{
	assert(fibril_mutex_is_locked(&services_list_mutex));

	list_foreach(namespaces_list, namespaces, loc_namespace_t, namespace) {
		if (namespace->id == id)
			return namespace;
	}

	return NULL;
}

/** Find service with given name. */
static loc_service_t *loc_service_find_name(const char *ns_name,
    const char *name)
{
	assert(fibril_mutex_is_locked(&services_list_mutex));

	list_foreach(services_list, services, loc_service_t, service) {
		if ((str_cmp(service->namespace->name, ns_name) == 0) &&
		    (str_cmp(service->name, name) == 0))
			return service;
	}

	return NULL;
}

/** Find service with given ID.
 *
 * @todo: use hash table
 *
 */
static loc_service_t *loc_service_find_id(service_id_t id)
{
	assert(fibril_mutex_is_locked(&services_list_mutex));

	list_foreach(services_list, services, loc_service_t, service) {
		if (service->id == id)
			return service;
	}

	return NULL;
}

/** Create a namespace (if not already present). */
static loc_namespace_t *loc_namespace_create(const char *ns_name)
{
	loc_namespace_t *namespace;

	assert(fibril_mutex_is_locked(&services_list_mutex));

	namespace = loc_namespace_find_name(ns_name);
	if (namespace != NULL)
		return namespace;

	namespace = (loc_namespace_t *) malloc(sizeof(loc_namespace_t));
	if (namespace == NULL)
		return NULL;

	namespace->name = str_dup(ns_name);
	if (namespace->name == NULL) {
		free(namespace);
		return NULL;
	}

	namespace->id = loc_create_id();
	namespace->refcnt = 0;

	/*
	 * Insert new namespace into list of registered namespaces
	 */
	list_append(&(namespace->namespaces), &namespaces_list);

	return namespace;
}

/** Destroy a namespace (if it is no longer needed). */
static void loc_namespace_destroy(loc_namespace_t *namespace)
{
	assert(fibril_mutex_is_locked(&services_list_mutex));

	if (namespace->refcnt == 0) {
		list_remove(&(namespace->namespaces));

		free(namespace->name);
		free(namespace);
	}
}

/** Increase namespace reference count by including service. */
static void loc_namespace_addref(loc_namespace_t *namespace,
    loc_service_t *service)
{
	assert(fibril_mutex_is_locked(&services_list_mutex));

	service->namespace = namespace;
	namespace->refcnt++;
}

/** Decrease namespace reference count. */
static void loc_namespace_delref(loc_namespace_t *namespace)
{
	assert(fibril_mutex_is_locked(&services_list_mutex));

	namespace->refcnt--;
	loc_namespace_destroy(namespace);
}

/** Unregister service and free it. */
static void loc_service_unregister_core(loc_service_t *service)
{
	assert(fibril_mutex_is_locked(&services_list_mutex));
	assert(fibril_mutex_is_locked(&cdir.mutex));

	loc_namespace_delref(service->namespace);
	list_remove(&(service->services));
	list_remove(&(service->server_services));

	/* Remove service from all categories. */
	while (!list_empty(&service->cat_memb)) {
		link_t *link = list_first(&service->cat_memb);
		svc_categ_t *memb = list_get_instance(link, svc_categ_t,
		    svc_link);
		category_t *cat = memb->cat;

		fibril_mutex_lock(&cat->mutex);
		category_remove_service(memb);
		fibril_mutex_unlock(&cat->mutex);
	}

	free(service->name);
	free(service);
}

/**
 * Read info about new server and add it into linked list of registered
 * servers.
 */
static loc_server_t *loc_server_register(void)
{
	ipc_call_t icall;
	async_get_call(&icall);

	if (ipc_get_imethod(&icall) != LOC_SERVER_REGISTER) {
		async_answer_0(&icall, EREFUSED);
		return NULL;
	}

	loc_server_t *server =
	    (loc_server_t *) malloc(sizeof(loc_server_t));
	if (server == NULL) {
		async_answer_0(&icall, ENOMEM);
		return NULL;
	}

	/*
	 * Get server name
	 */
	errno_t rc = async_data_write_accept((void **) &server->name, true, 0,
	    LOC_NAME_MAXLEN, 0, NULL);
	if (rc != EOK) {
		free(server);
		async_answer_0(&icall, rc);
		return NULL;
	}

	/*
	 * Create connection to the server
	 */
	server->sess = async_callback_receive(EXCHANGE_SERIALIZE);
	if (!server->sess) {
		free(server->name);
		free(server);
		async_answer_0(&icall, ENOTSUP);
		return NULL;
	}

	/*
	 * Initialize mutex for list of services
	 * supplied by this server
	 */
	fibril_mutex_initialize(&server->services_mutex);

	/*
	 * Initialize list of supplied services
	 */
	list_initialize(&server->services);
	link_initialize(&server->servers);

	fibril_mutex_lock(&servers_list_mutex);

	/*
	 * TODO:
	 * Check that no server with name equal to
	 * server->name is registered
	 */

	/*
	 * Insert new server into list of registered servers
	 */
	list_append(&(server->servers), &servers_list);
	fibril_mutex_unlock(&servers_list_mutex);

	async_answer_0(&icall, EOK);

	return server;
}

/**
 * Unregister server, unregister all its services and free server
 * structure.
 *
 */
static errno_t loc_server_unregister(loc_server_t *server)
{
	if (server == NULL)
		return EEXIST;

	fibril_mutex_lock(&servers_list_mutex);

	if (server->sess)
		async_hangup(server->sess);

	/* Remove it from list of servers */
	list_remove(&(server->servers));

	/* Unregister all its services */
	fibril_mutex_lock(&services_list_mutex);
	fibril_mutex_lock(&server->services_mutex);
	fibril_mutex_lock(&cdir.mutex);

	while (!list_empty(&server->services)) {
		loc_service_t *service = list_get_instance(
		    list_first(&server->services), loc_service_t,
		    server_services);
		loc_service_unregister_core(service);
	}

	fibril_mutex_unlock(&cdir.mutex);
	fibril_mutex_unlock(&server->services_mutex);
	fibril_mutex_unlock(&services_list_mutex);
	fibril_mutex_unlock(&servers_list_mutex);

	/* Free name and server */
	if (server->name != NULL)
		free(server->name);

	free(server);

	loc_category_change_event();
	return EOK;
}

/** Register service
 *
 */
static void loc_service_register(ipc_call_t *icall, loc_server_t *server)
{
	if (server == NULL) {
		async_answer_0(icall, EREFUSED);
		return;
	}

	/* Create new service entry */
	loc_service_t *service =
	    (loc_service_t *) malloc(sizeof(loc_service_t));
	if (service == NULL) {
		async_answer_0(icall, ENOMEM);
		return;
	}

	/* Get fqsn */
	char *fqsn;
	errno_t rc = async_data_write_accept((void **) &fqsn, true, 0,
	    LOC_NAME_MAXLEN, 0, NULL);
	if (rc != EOK) {
		free(service);
		async_answer_0(icall, rc);
		return;
	}

	char *ns_name;
	if (!loc_fqsn_split(fqsn, &ns_name, &service->name)) {
		free(fqsn);
		free(service);
		async_answer_0(icall, EINVAL);
		return;
	}

	free(fqsn);

	fibril_mutex_lock(&services_list_mutex);

	loc_namespace_t *namespace = loc_namespace_create(ns_name);
	free(ns_name);
	if (namespace == NULL) {
		fibril_mutex_unlock(&services_list_mutex);
		free(service->name);
		free(service);
		async_answer_0(icall, ENOMEM);
		return;
	}

	link_initialize(&service->services);
	link_initialize(&service->server_services);
	list_initialize(&service->cat_memb);

	/* Check that service is not already registered */
	if (loc_service_find_name(namespace->name, service->name) != NULL) {
		printf("%s: Service '%s/%s' already registered\n", NAME,
		    namespace->name, service->name);
		loc_namespace_destroy(namespace);
		fibril_mutex_unlock(&services_list_mutex);
		free(service->name);
		free(service);
		async_answer_0(icall, EEXIST);
		return;
	}

	/* Get unique service ID */
	service->id = loc_create_id();

	loc_namespace_addref(namespace, service);
	service->server = server;

	/* Insert service into list of all services  */
	list_append(&service->services, &services_list);

	/* Insert service into list of services supplied by one server */
	fibril_mutex_lock(&service->server->services_mutex);

	list_append(&service->server_services, &service->server->services);

	fibril_mutex_unlock(&service->server->services_mutex);
	fibril_condvar_broadcast(&services_list_cv);
	fibril_mutex_unlock(&services_list_mutex);

	async_answer_1(icall, EOK, service->id);
}

/**
 *
 */
static void loc_service_unregister(ipc_call_t *icall, loc_server_t *server)
{
	loc_service_t *svc;

	fibril_mutex_lock(&services_list_mutex);
	svc = loc_service_find_id(ipc_get_arg1(icall));
	if (svc == NULL) {
		fibril_mutex_unlock(&services_list_mutex);
		async_answer_0(icall, ENOENT);
		return;
	}

	fibril_mutex_lock(&cdir.mutex);
	loc_service_unregister_core(svc);
	fibril_mutex_unlock(&cdir.mutex);
	fibril_mutex_unlock(&services_list_mutex);

	/*
	 * First send out all notifications and only then answer the request.
	 * Otherwise the current fibril might block and transitively wait for
	 * the completion of requests that are routed to it via an IPC loop.
	 */
	loc_category_change_event();
	async_answer_0(icall, EOK);
}

static void loc_category_get_name(ipc_call_t *icall)
{
	ipc_call_t call;
	size_t size;
	size_t act_size;
	category_t *cat;

	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	fibril_mutex_lock(&cdir.mutex);

	cat = category_get(&cdir, ipc_get_arg1(icall));
	if (cat == NULL) {
		fibril_mutex_unlock(&cdir.mutex);
		async_answer_0(&call, ENOENT);
		async_answer_0(icall, ENOENT);
		return;
	}

	act_size = str_size(cat->name);
	if (act_size > size) {
		fibril_mutex_unlock(&cdir.mutex);
		async_answer_0(&call, EOVERFLOW);
		async_answer_0(icall, EOVERFLOW);
		return;
	}

	errno_t retval = async_data_read_finalize(&call, cat->name,
	    min(size, act_size));

	fibril_mutex_unlock(&cdir.mutex);

	async_answer_0(icall, retval);
}

static void loc_service_get_name(ipc_call_t *icall)
{
	ipc_call_t call;
	size_t size;
	size_t act_size;
	loc_service_t *svc;
	char *fqn;

	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	fibril_mutex_lock(&services_list_mutex);

	svc = loc_service_find_id(ipc_get_arg1(icall));
	if (svc == NULL) {
		fibril_mutex_unlock(&services_list_mutex);
		async_answer_0(&call, ENOENT);
		async_answer_0(icall, ENOENT);
		return;
	}

	if (asprintf(&fqn, "%s/%s", svc->namespace->name, svc->name) < 0) {
		fibril_mutex_unlock(&services_list_mutex);
		async_answer_0(&call, ENOMEM);
		async_answer_0(icall, ENOMEM);
		return;
	}

	act_size = str_size(fqn);
	if (act_size > size) {
		free(fqn);
		fibril_mutex_unlock(&services_list_mutex);
		async_answer_0(&call, EOVERFLOW);
		async_answer_0(icall, EOVERFLOW);
		return;
	}

	errno_t retval = async_data_read_finalize(&call, fqn,
	    min(size, act_size));
	free(fqn);

	fibril_mutex_unlock(&services_list_mutex);

	async_answer_0(icall, retval);
}

static void loc_service_get_server_name(ipc_call_t *icall)
{
	ipc_call_t call;
	size_t size;
	size_t act_size;
	loc_service_t *svc;

	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	fibril_mutex_lock(&services_list_mutex);

	svc = loc_service_find_id(ipc_get_arg1(icall));
	if (svc == NULL) {
		fibril_mutex_unlock(&services_list_mutex);
		async_answer_0(&call, ENOENT);
		async_answer_0(icall, ENOENT);
		return;
	}

	if (svc->server == NULL) {
		fibril_mutex_unlock(&services_list_mutex);
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	act_size = str_size(svc->server->name);
	if (act_size > size) {
		fibril_mutex_unlock(&services_list_mutex);
		async_answer_0(&call, EOVERFLOW);
		async_answer_0(icall, EOVERFLOW);
		return;
	}

	errno_t retval = async_data_read_finalize(&call, svc->server->name,
	    min(size, act_size));

	fibril_mutex_unlock(&services_list_mutex);

	async_answer_0(icall, retval);
}

/** Connect client to the service.
 *
 * Find server supplying requested service and forward
 * the message to it.
 *
 */
static void loc_forward(ipc_call_t *call, void *arg)
{
	fibril_mutex_lock(&services_list_mutex);

	/*
	 * Get ID from request
	 */
	iface_t iface = ipc_get_arg1(call);
	service_id_t id = ipc_get_arg2(call);
	loc_service_t *svc = loc_service_find_id(id);

	if ((svc == NULL) || (svc->server == NULL) || (!svc->server->sess)) {
		fibril_mutex_unlock(&services_list_mutex);
		async_answer_0(call, ENOENT);
		return;
	}

	async_exch_t *exch = async_exchange_begin(svc->server->sess);
	async_forward_1(call, exch, iface, svc->id, IPC_FF_NONE);
	async_exchange_end(exch);

	fibril_mutex_unlock(&services_list_mutex);
}

/** Find ID for service identified by name.
 *
 * In answer will be send EOK and service ID in arg1 or a error
 * code from errno.h.
 *
 */
static void loc_service_get_id(ipc_call_t *icall)
{
	char *fqsn;

	/* Get fqsn */
	errno_t rc = async_data_write_accept((void **) &fqsn, true, 0,
	    LOC_NAME_MAXLEN, 0, NULL);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	char *ns_name;
	char *name;
	if (!loc_fqsn_split(fqsn, &ns_name, &name)) {
		free(fqsn);
		async_answer_0(icall, EINVAL);
		return;
	}

	free(fqsn);

	fibril_mutex_lock(&services_list_mutex);
	const loc_service_t *svc;

recheck:

	/*
	 * Find service name in the list of known services.
	 */
	svc = loc_service_find_name(ns_name, name);

	/*
	 * Device was not found.
	 */
	if (svc == NULL) {
		if (ipc_get_arg1(icall) & IPC_FLAG_BLOCKING) {
			/* Blocking lookup */
			fibril_condvar_wait(&services_list_cv,
			    &services_list_mutex);
			goto recheck;
		}

		async_answer_0(icall, ENOENT);
		free(ns_name);
		free(name);
		fibril_mutex_unlock(&services_list_mutex);
		return;
	}

	async_answer_1(icall, EOK, svc->id);

	fibril_mutex_unlock(&services_list_mutex);
	free(ns_name);
	free(name);
}

/** Find ID for namespace identified by name.
 *
 * In answer will be send EOK and service ID in arg1 or a error
 * code from errno.h.
 *
 */
static void loc_namespace_get_id(ipc_call_t *icall)
{
	char *name;

	/* Get service name */
	errno_t rc = async_data_write_accept((void **) &name, true, 0,
	    LOC_NAME_MAXLEN, 0, NULL);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	fibril_mutex_lock(&services_list_mutex);
	const loc_namespace_t *namespace;

recheck:

	/*
	 * Find namespace name in the list of known namespaces.
	 */
	namespace = loc_namespace_find_name(name);

	/*
	 * Namespace was not found.
	 */
	if (namespace == NULL) {
		if (ipc_get_arg1(icall) & IPC_FLAG_BLOCKING) {
			/* Blocking lookup */
			fibril_condvar_wait(&services_list_cv,
			    &services_list_mutex);
			goto recheck;
		}

		async_answer_0(icall, ENOENT);
		free(name);
		fibril_mutex_unlock(&services_list_mutex);
		return;
	}

	async_answer_1(icall, EOK, namespace->id);

	fibril_mutex_unlock(&services_list_mutex);
	free(name);
}

/** Create callback connection.
 *
 * Create callback connection which will be used to send category change
 * events.
 *
 * On success, answer will contain EOK errno_t retval.
 * On failure, error code will be sent in retval.
 *
 */
static void loc_callback_create(ipc_call_t *icall)
{
	cb_sess_t *cb_sess = calloc(1, sizeof(cb_sess_t));
	if (cb_sess == NULL) {
		async_answer_0(icall, ENOMEM);
		return;
	}

	async_sess_t *sess = async_callback_receive(EXCHANGE_SERIALIZE);
	if (sess == NULL) {
		free(cb_sess);
		async_answer_0(icall, ENOMEM);
		return;
	}

	cb_sess->sess = sess;
	link_initialize(&cb_sess->cb_sess_list);

	fibril_mutex_lock(&callback_sess_mutex);
	list_append(&cb_sess->cb_sess_list, &callback_sess_list);
	fibril_mutex_unlock(&callback_sess_mutex);

	async_answer_0(icall, EOK);
}

void loc_category_change_event(void)
{
	fibril_mutex_lock(&callback_sess_mutex);

	list_foreach(callback_sess_list, cb_sess_list, cb_sess_t, cb_sess) {
		async_exch_t *exch = async_exchange_begin(cb_sess->sess);
		async_msg_0(exch, LOC_EVENT_CAT_CHANGE);
		async_exchange_end(exch);
	}

	fibril_mutex_unlock(&callback_sess_mutex);
}

/** Find ID for category specified by name.
 *
 * On success, answer will contain EOK errno_t retval and service ID in arg1.
 * On failure, error code will be sent in retval.
 *
 */
static void loc_category_get_id(ipc_call_t *icall)
{
	char *name;
	category_t *cat;

	/* Get service name */
	errno_t rc = async_data_write_accept((void **) &name, true, 0,
	    LOC_NAME_MAXLEN, 0, NULL);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	fibril_mutex_lock(&cdir.mutex);

	cat = category_find_by_name(&cdir, name);
	if (cat == NULL) {
		/* Category not found */
		async_answer_0(icall, ENOENT);
		goto cleanup;
	}

	async_answer_1(icall, EOK, cat->id);
cleanup:
	fibril_mutex_unlock(&cdir.mutex);
	free(name);
}

static void loc_id_probe(ipc_call_t *icall)
{
	fibril_mutex_lock(&services_list_mutex);

	loc_namespace_t *namespace =
	    loc_namespace_find_id(ipc_get_arg1(icall));
	if (namespace == NULL) {
		loc_service_t *svc =
		    loc_service_find_id(ipc_get_arg1(icall));
		if (svc == NULL)
			async_answer_1(icall, EOK, LOC_OBJECT_NONE);
		else
			async_answer_1(icall, EOK, LOC_OBJECT_SERVICE);
	} else
		async_answer_1(icall, EOK, LOC_OBJECT_NAMESPACE);

	fibril_mutex_unlock(&services_list_mutex);
}

static void loc_get_namespace_count(ipc_call_t *icall)
{
	fibril_mutex_lock(&services_list_mutex);
	async_answer_1(icall, EOK, list_count(&namespaces_list));
	fibril_mutex_unlock(&services_list_mutex);
}

static void loc_get_service_count(ipc_call_t *icall)
{
	fibril_mutex_lock(&services_list_mutex);

	loc_namespace_t *namespace =
	    loc_namespace_find_id(ipc_get_arg1(icall));
	if (namespace == NULL)
		async_answer_0(icall, EEXIST);
	else
		async_answer_1(icall, EOK, namespace->refcnt);

	fibril_mutex_unlock(&services_list_mutex);
}

static void loc_get_categories(ipc_call_t *icall)
{
	ipc_call_t call;
	size_t size;
	size_t act_size;
	errno_t rc;

	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	category_id_t *id_buf = (category_id_t *) malloc(size);
	if (id_buf == NULL) {
		fibril_mutex_unlock(&cdir.mutex);
		async_answer_0(&call, ENOMEM);
		async_answer_0(icall, ENOMEM);
		return;
	}

	fibril_mutex_lock(&cdir.mutex);

	rc = categ_dir_get_categories(&cdir, id_buf, size, &act_size);
	if (rc != EOK) {
		fibril_mutex_unlock(&cdir.mutex);
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	fibril_mutex_unlock(&cdir.mutex);

	errno_t retval = async_data_read_finalize(&call, id_buf, size);
	free(id_buf);

	async_answer_1(icall, retval, act_size);
}

static void loc_get_namespaces(ipc_call_t *icall)
{
	ipc_call_t call;
	size_t size;
	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if ((size % sizeof(loc_sdesc_t)) != 0) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	fibril_mutex_lock(&services_list_mutex);

	size_t count = size / sizeof(loc_sdesc_t);
	if (count != list_count(&namespaces_list)) {
		fibril_mutex_unlock(&services_list_mutex);
		async_answer_0(&call, EOVERFLOW);
		async_answer_0(icall, EOVERFLOW);
		return;
	}

	loc_sdesc_t *desc = (loc_sdesc_t *) malloc(size);
	if (desc == NULL) {
		fibril_mutex_unlock(&services_list_mutex);
		async_answer_0(&call, ENOMEM);
		async_answer_0(icall, ENOMEM);
		return;
	}

	size_t pos = 0;
	list_foreach(namespaces_list, namespaces, loc_namespace_t, namespace) {
		desc[pos].id = namespace->id;
		str_cpy(desc[pos].name, LOC_NAME_MAXLEN, namespace->name);
		pos++;
	}

	errno_t retval = async_data_read_finalize(&call, desc, size);

	free(desc);
	fibril_mutex_unlock(&services_list_mutex);

	async_answer_0(icall, retval);
}

static void loc_get_services(ipc_call_t *icall)
{
	/*
	 * FIXME: Use faster algorithm which can make better use
	 * of namespaces
	 */

	ipc_call_t call;
	size_t size;
	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if ((size % sizeof(loc_sdesc_t)) != 0) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	fibril_mutex_lock(&services_list_mutex);

	loc_namespace_t *namespace =
	    loc_namespace_find_id(ipc_get_arg1(icall));
	if (namespace == NULL) {
		fibril_mutex_unlock(&services_list_mutex);
		async_answer_0(&call, ENOENT);
		async_answer_0(icall, ENOENT);
		return;
	}

	size_t count = size / sizeof(loc_sdesc_t);
	if (count != namespace->refcnt) {
		fibril_mutex_unlock(&services_list_mutex);
		async_answer_0(&call, EOVERFLOW);
		async_answer_0(icall, EOVERFLOW);
		return;
	}

	loc_sdesc_t *desc = (loc_sdesc_t *) malloc(size);
	if (desc == NULL) {
		fibril_mutex_unlock(&services_list_mutex);
		async_answer_0(&call, ENOMEM);
		async_answer_0(icall, EREFUSED);
		return;
	}

	size_t pos = 0;
	list_foreach(services_list, services, loc_service_t, service) {
		if (service->namespace == namespace) {
			desc[pos].id = service->id;
			str_cpy(desc[pos].name, LOC_NAME_MAXLEN, service->name);
			pos++;
		}
	}

	errno_t retval = async_data_read_finalize(&call, desc, size);

	free(desc);
	fibril_mutex_unlock(&services_list_mutex);

	async_answer_0(icall, retval);
}

static void loc_category_get_svcs(ipc_call_t *icall)
{
	ipc_call_t call;
	size_t size;
	size_t act_size;
	errno_t rc;

	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	fibril_mutex_lock(&cdir.mutex);

	category_t *cat = category_get(&cdir, ipc_get_arg1(icall));
	if (cat == NULL) {
		fibril_mutex_unlock(&cdir.mutex);
		async_answer_0(&call, ENOENT);
		async_answer_0(icall, ENOENT);
		return;
	}

	category_id_t *id_buf = (category_id_t *) malloc(size);
	if (id_buf == NULL) {
		fibril_mutex_unlock(&cdir.mutex);
		async_answer_0(&call, ENOMEM);
		async_answer_0(icall, ENOMEM);
		return;
	}

	fibril_mutex_lock(&cat->mutex);

	rc = category_get_services(cat, id_buf, size, &act_size);
	if (rc != EOK) {
		fibril_mutex_unlock(&cat->mutex);
		fibril_mutex_unlock(&cdir.mutex);
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	fibril_mutex_unlock(&cat->mutex);
	fibril_mutex_unlock(&cdir.mutex);

	errno_t retval = async_data_read_finalize(&call, id_buf, size);
	free(id_buf);

	async_answer_1(icall, retval, act_size);
}

static void loc_null_create(ipc_call_t *icall)
{
	fibril_mutex_lock(&null_services_mutex);

	unsigned int i;
	bool fnd = false;

	for (i = 0; i < NULL_SERVICES; i++) {
		if (null_services[i] == NULL) {
			fnd = true;
			break;
		}
	}

	if (!fnd) {
		fibril_mutex_unlock(&null_services_mutex);
		async_answer_0(icall, ENOMEM);
		return;
	}

	char null[LOC_NAME_MAXLEN];
	snprintf(null, LOC_NAME_MAXLEN, "%u", i);

	char *dev_name = str_dup(null);
	if (dev_name == NULL) {
		fibril_mutex_unlock(&null_services_mutex);
		async_answer_0(icall, ENOMEM);
		return;
	}

	loc_service_t *service =
	    (loc_service_t *) malloc(sizeof(loc_service_t));
	if (service == NULL) {
		fibril_mutex_unlock(&null_services_mutex);
		async_answer_0(icall, ENOMEM);
		return;
	}

	fibril_mutex_lock(&services_list_mutex);

	loc_namespace_t *namespace = loc_namespace_create("null");
	if (namespace == NULL) {
		fibril_mutex_lock(&services_list_mutex);
		fibril_mutex_unlock(&null_services_mutex);
		async_answer_0(icall, ENOMEM);
		return;
	}

	link_initialize(&service->services);
	link_initialize(&service->server_services);
	list_initialize(&service->cat_memb);

	/* Get unique service ID */
	service->id = loc_create_id();
	service->server = NULL;

	loc_namespace_addref(namespace, service);
	service->name = dev_name;

	/*
	 * Insert service into list of all services and into null services array.
	 * Insert service into a dummy list of null server's services so that it
	 * can be safely removed later.
	 */
	list_append(&service->services, &services_list);
	list_append(&service->server_services, &dummy_null_services);
	null_services[i] = service;

	fibril_mutex_unlock(&services_list_mutex);
	fibril_mutex_unlock(&null_services_mutex);

	async_answer_1(icall, EOK, (sysarg_t) i);
}

static void loc_null_destroy(ipc_call_t *icall)
{
	sysarg_t i = ipc_get_arg1(icall);
	if (i >= NULL_SERVICES) {
		async_answer_0(icall, ELIMIT);
		return;
	}

	fibril_mutex_lock(&null_services_mutex);

	if (null_services[i] == NULL) {
		fibril_mutex_unlock(&null_services_mutex);
		async_answer_0(icall, ENOENT);
		return;
	}

	fibril_mutex_lock(&services_list_mutex);
	fibril_mutex_lock(&cdir.mutex);
	loc_service_unregister_core(null_services[i]);
	fibril_mutex_unlock(&cdir.mutex);
	fibril_mutex_unlock(&services_list_mutex);

	null_services[i] = NULL;

	fibril_mutex_unlock(&null_services_mutex);
	async_answer_0(icall, EOK);
}

static void loc_service_add_to_cat(ipc_call_t *icall)
{
	category_t *cat;
	loc_service_t *svc;
	catid_t cat_id;
	service_id_t svc_id;
	errno_t retval;

	svc_id = ipc_get_arg1(icall);
	cat_id = ipc_get_arg2(icall);

	fibril_mutex_lock(&services_list_mutex);
	fibril_mutex_lock(&cdir.mutex);

	cat = category_get(&cdir, cat_id);
	svc = loc_service_find_id(svc_id);

	if (cat == NULL || svc == NULL) {
		fibril_mutex_unlock(&cdir.mutex);
		fibril_mutex_unlock(&services_list_mutex);
		async_answer_0(icall, ENOENT);
		return;
	}

	fibril_mutex_lock(&cat->mutex);
	retval = category_add_service(cat, svc);

	fibril_mutex_unlock(&cat->mutex);
	fibril_mutex_unlock(&cdir.mutex);
	fibril_mutex_unlock(&services_list_mutex);

	/*
	 * First send out all notifications and only then answer the request.
	 * Otherwise the current fibril might block and transitively wait for
	 * the completion of requests that are routed to it via an IPC loop.
	 */
	loc_category_change_event();
	async_answer_0(icall, retval);
}

/** Initialize location service.
 *
 *
 */
static bool loc_init(void)
{
	unsigned int i;
	category_t *cat;

	for (i = 0; i < NULL_SERVICES; i++)
		null_services[i] = NULL;

	categ_dir_init(&cdir);

	cat = category_new("disk");
	categ_dir_add_cat(&cdir, cat);

	cat = category_new("partition");
	categ_dir_add_cat(&cdir, cat);

	cat = category_new("iplink");
	categ_dir_add_cat(&cdir, cat);

	cat = category_new("keyboard");
	categ_dir_add_cat(&cdir, cat);

	cat = category_new("mouse");
	categ_dir_add_cat(&cdir, cat);

	cat = category_new("led");
	categ_dir_add_cat(&cdir, cat);

	cat = category_new("serial");
	categ_dir_add_cat(&cdir, cat);

	cat = category_new("console");
	categ_dir_add_cat(&cdir, cat);

	cat = category_new("clock");
	categ_dir_add_cat(&cdir, cat);

	cat = category_new("tbarcfg-notif");
	categ_dir_add_cat(&cdir, cat);

	cat = category_new("test3");
	categ_dir_add_cat(&cdir, cat);

	cat = category_new("usbdiag");
	categ_dir_add_cat(&cdir, cat);

	cat = category_new("usbhc");
	categ_dir_add_cat(&cdir, cat);

	cat = category_new("virt-null");
	categ_dir_add_cat(&cdir, cat);

	cat = category_new("virtual");
	categ_dir_add_cat(&cdir, cat);

	cat = category_new("nic");
	categ_dir_add_cat(&cdir, cat);

	cat = category_new("ieee80211");
	categ_dir_add_cat(&cdir, cat);

	cat = category_new("irc");
	categ_dir_add_cat(&cdir, cat);

	cat = category_new("display-device");
	categ_dir_add_cat(&cdir, cat);

	cat = category_new("audio-pcm");
	categ_dir_add_cat(&cdir, cat);

	cat = category_new("printer-port");
	categ_dir_add_cat(&cdir, cat);

	cat = category_new("pci");
	categ_dir_add_cat(&cdir, cat);

	return true;
}

/** Handle connection on supplier port.
 *
 */
static void loc_connection_supplier(ipc_call_t *icall, void *arg)
{
	/* Accept connection */
	async_accept_0(icall);

	/*
	 * Each connection begins by a LOC_SERVER_REGISTER, which precludes us
	 * from using parallel exchanges.
	 */
	static_assert((INTERFACE_LOC_SUPPLIER & IFACE_EXCHANGE_MASK) ==
	    IFACE_EXCHANGE_SERIALIZE, "");

	loc_server_t *server = loc_server_register();
	if (server == NULL)
		return;

	while (true) {
		ipc_call_t call;
		async_get_call(&call);

		if (!ipc_get_imethod(&call)) {
			async_answer_0(&call, EOK);
			break;
		}

		switch (ipc_get_imethod(&call)) {
		case LOC_SERVER_UNREGISTER:
			if (server == NULL)
				async_answer_0(&call, ENOENT);
			else
				async_answer_0(&call, EOK);
			break;
		case LOC_SERVICE_ADD_TO_CAT:
			/* Add service to category */
			loc_service_add_to_cat(&call);
			break;
		case LOC_SERVICE_REGISTER:
			/* Register one service */
			loc_service_register(&call, server);
			break;
		case LOC_SERVICE_UNREGISTER:
			/* Remove one service */
			loc_service_unregister(&call, server);
			break;
		case LOC_SERVICE_GET_ID:
			loc_service_get_id(&call);
			break;
		case LOC_NAMESPACE_GET_ID:
			loc_namespace_get_id(&call);
			break;
		default:
			async_answer_0(&call, ENOENT);
		}
	}

	if (server != NULL) {
		/*
		 * Unregister the server and all its services.
		 */
		loc_server_unregister(server);
		server = NULL;
	}
}

/** Handle connection on consumer port.
 *
 */
static void loc_connection_consumer(ipc_call_t *icall, void *arg)
{
	/* Accept connection */
	async_accept_0(icall);

	while (true) {
		ipc_call_t call;
		async_get_call(&call);

		if (!ipc_get_imethod(&call)) {
			async_answer_0(&call, EOK);
			break;
		}

		switch (ipc_get_imethod(&call)) {
		case LOC_SERVICE_GET_ID:
			loc_service_get_id(&call);
			break;
		case LOC_SERVICE_GET_NAME:
			loc_service_get_name(&call);
			break;
		case LOC_SERVICE_GET_SERVER_NAME:
			loc_service_get_server_name(&call);
			break;
		case LOC_NAMESPACE_GET_ID:
			loc_namespace_get_id(&call);
			break;
		case LOC_CALLBACK_CREATE:
			loc_callback_create(&call);
			break;
		case LOC_CATEGORY_GET_ID:
			loc_category_get_id(&call);
			break;
		case LOC_CATEGORY_GET_NAME:
			loc_category_get_name(&call);
			break;
		case LOC_CATEGORY_GET_SVCS:
			loc_category_get_svcs(&call);
			break;
		case LOC_ID_PROBE:
			loc_id_probe(&call);
			break;
		case LOC_NULL_CREATE:
			loc_null_create(&call);
			break;
		case LOC_NULL_DESTROY:
			loc_null_destroy(&call);
			break;
		case LOC_GET_NAMESPACE_COUNT:
			loc_get_namespace_count(&call);
			break;
		case LOC_GET_SERVICE_COUNT:
			loc_get_service_count(&call);
			break;
		case LOC_GET_CATEGORIES:
			loc_get_categories(&call);
			break;
		case LOC_GET_NAMESPACES:
			loc_get_namespaces(&call);
			break;
		case LOC_GET_SERVICES:
			loc_get_services(&call);
			break;
		default:
			async_answer_0(&call, ENOENT);
		}
	}
}

/**
 *
 */
int main(int argc, char *argv[])
{
	printf("%s: HelenOS Location Service\n", NAME);

	if (!loc_init()) {
		printf("%s: Error while initializing service\n", NAME);
		return -1;
	}

	/* Register location service at naming service */
	errno_t rc = service_register(SERVICE_LOC, INTERFACE_LOC_SUPPLIER,
	    loc_connection_supplier, NULL);
	if (rc != EOK) {
		printf("%s: Error while registering supplier service: %s\n", NAME, str_error(rc));
		return rc;
	}

	rc = service_register(SERVICE_LOC, INTERFACE_LOC_CONSUMER,
	    loc_connection_consumer, NULL);
	if (rc != EOK) {
		printf("%s: Error while registering consumer service: %s\n", NAME, str_error(rc));
		return rc;
	}

	rc = service_register_broker(SERVICE_LOC, loc_forward, NULL);
	if (rc != EOK) {
		printf("%s: Error while registering broker service: %s\n", NAME, str_error(rc));
		return rc;
	}

	printf("%s: Accepting connections\n", NAME);
	async_manager();

	/* Never reached */
	return 0;
}

/**
 * @}
 */
