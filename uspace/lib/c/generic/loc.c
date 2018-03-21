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

#include <str.h>
#include <ipc/services.h>
#include <ns.h>
#include <ipc/loc.h>
#include <loc.h>
#include <fibril_synch.h>
#include <async.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>

static FIBRIL_MUTEX_INITIALIZE(loc_supp_block_mutex);
static FIBRIL_MUTEX_INITIALIZE(loc_cons_block_mutex);

static FIBRIL_MUTEX_INITIALIZE(loc_supplier_mutex);
static FIBRIL_MUTEX_INITIALIZE(loc_consumer_mutex);

static FIBRIL_MUTEX_INITIALIZE(loc_callback_mutex);
static bool loc_callback_created = false;
static loc_cat_change_cb_t cat_change_cb = NULL;

static async_sess_t *loc_supp_block_sess = NULL;
static async_sess_t *loc_cons_block_sess = NULL;

static async_sess_t *loc_supplier_sess = NULL;
static async_sess_t *loc_consumer_sess = NULL;

static void loc_cb_conn(cap_call_handle_t icall_handle, ipc_call_t *icall, void *arg)
{
	while (true) {
		ipc_call_t call;
		cap_call_handle_t chandle = async_get_call(&call);

		if (!IPC_GET_IMETHOD(call)) {
			/* TODO: Handle hangup */
			return;
		}

		switch (IPC_GET_IMETHOD(call)) {
		case LOC_EVENT_CAT_CHANGE:
			fibril_mutex_lock(&loc_callback_mutex);
			loc_cat_change_cb_t cb_fun = cat_change_cb;
			fibril_mutex_unlock(&loc_callback_mutex);

			async_answer_0(chandle, EOK);

			if (cb_fun != NULL)
				(*cb_fun)();

			break;
		default:
			async_answer_0(chandle, ENOTSUP);
		}
	}
}


static void clone_session(fibril_mutex_t *mtx, async_sess_t *src,
    async_sess_t **dst)
{
	fibril_mutex_lock(mtx);

	if ((*dst == NULL) && (src != NULL))
		*dst = src;

	fibril_mutex_unlock(mtx);
}

/** Create callback
 *
 * Must be called with loc_callback_mutex locked.
 *
 * @return EOK on success.
 *
 */
static errno_t loc_callback_create(void)
{
	if (!loc_callback_created) {
		async_exch_t *exch =
		    loc_exchange_begin_blocking(INTERFACE_LOC_CONSUMER);

		ipc_call_t answer;
		aid_t req = async_send_0(exch, LOC_CALLBACK_CREATE, &answer);

		port_id_t port;
		errno_t rc = async_create_callback_port(exch, INTERFACE_LOC_CB, 0, 0,
		    loc_cb_conn, NULL, &port);

		loc_exchange_end(exch);

		if (rc != EOK)
			return rc;

		errno_t retval;
		async_wait_for(req, &retval);
		if (retval != EOK)
			return retval;

		loc_callback_created = true;
	}

	return EOK;
}

/** Start an async exchange on the loc session (blocking).
 *
 * @param iface Location service interface to choose
 *
 * @return New exchange.
 *
 */
async_exch_t *loc_exchange_begin_blocking(iface_t iface)
{
	switch (iface) {
	case INTERFACE_LOC_SUPPLIER:
		fibril_mutex_lock(&loc_supp_block_mutex);

		while (loc_supp_block_sess == NULL) {
			clone_session(&loc_supplier_mutex, loc_supplier_sess,
			    &loc_supp_block_sess);

			if (loc_supp_block_sess == NULL)
				loc_supp_block_sess =
				    service_connect_blocking(SERVICE_LOC,
				    INTERFACE_LOC_SUPPLIER, 0);
		}

		fibril_mutex_unlock(&loc_supp_block_mutex);

		clone_session(&loc_supplier_mutex, loc_supp_block_sess,
		    &loc_supplier_sess);

		return async_exchange_begin(loc_supp_block_sess);
	case INTERFACE_LOC_CONSUMER:
		fibril_mutex_lock(&loc_cons_block_mutex);

		while (loc_cons_block_sess == NULL) {
			clone_session(&loc_consumer_mutex, loc_consumer_sess,
			    &loc_cons_block_sess);

			if (loc_cons_block_sess == NULL)
				loc_cons_block_sess =
				    service_connect_blocking(SERVICE_LOC,
				    INTERFACE_LOC_CONSUMER, 0);
		}

		fibril_mutex_unlock(&loc_cons_block_mutex);

		clone_session(&loc_consumer_mutex, loc_cons_block_sess,
		    &loc_consumer_sess);

		return async_exchange_begin(loc_cons_block_sess);
	default:
		return NULL;
	}
}

/** Start an async exchange on the loc session.
 *
 * @param iface Location service interface to choose
 *
 * @return New exchange.
 *
 */
async_exch_t *loc_exchange_begin(iface_t iface)
{
	switch (iface) {
	case INTERFACE_LOC_SUPPLIER:
		fibril_mutex_lock(&loc_supplier_mutex);

		if (loc_supplier_sess == NULL)
			loc_supplier_sess =
			    service_connect(SERVICE_LOC,
			    INTERFACE_LOC_SUPPLIER, 0);

		fibril_mutex_unlock(&loc_supplier_mutex);

		if (loc_supplier_sess == NULL)
			return NULL;

		return async_exchange_begin(loc_supplier_sess);
	case INTERFACE_LOC_CONSUMER:
		fibril_mutex_lock(&loc_consumer_mutex);

		if (loc_consumer_sess == NULL)
			loc_consumer_sess =
			    service_connect(SERVICE_LOC,
			    INTERFACE_LOC_CONSUMER, 0);

		fibril_mutex_unlock(&loc_consumer_mutex);

		if (loc_consumer_sess == NULL)
			return NULL;

		return async_exchange_begin(loc_consumer_sess);
	default:
		return NULL;
	}
}

/** Finish an async exchange on the loc session.
 *
 * @param exch Exchange to be finished.
 *
 */
void loc_exchange_end(async_exch_t *exch)
{
	async_exchange_end(exch);
}

/** Register new driver with loc. */
errno_t loc_server_register(const char *name)
{
	async_exch_t *exch = loc_exchange_begin_blocking(INTERFACE_LOC_SUPPLIER);

	ipc_call_t answer;
	aid_t req = async_send_2(exch, LOC_SERVER_REGISTER, 0, 0, &answer);
	errno_t retval = async_data_write_start(exch, name, str_size(name));

	if (retval != EOK) {
		async_forget(req);
		loc_exchange_end(exch);
		return retval;
	}

	async_connect_to_me(exch, 0, 0, 0);

	/*
	 * First wait for the answer and then end the exchange. The opposite
	 * order is generally wrong because it may lead to a deadlock under
	 * certain circumstances.
	 */
	async_wait_for(req, &retval);
	loc_exchange_end(exch);

	return retval;
}

/** Register new service.
 *
 * @param      fqsn  Fully qualified service name
 * @param[out] sid   Service ID of new service
 *
 */
errno_t loc_service_register(const char *fqsn, service_id_t *sid)
{
	async_exch_t *exch = loc_exchange_begin_blocking(INTERFACE_LOC_SUPPLIER);

	ipc_call_t answer;
	aid_t req = async_send_0(exch, LOC_SERVICE_REGISTER, &answer);
	errno_t retval = async_data_write_start(exch, fqsn, str_size(fqsn));

	if (retval != EOK) {
		async_forget(req);
		loc_exchange_end(exch);
		return retval;
	}

	/*
	 * First wait for the answer and then end the exchange. The opposite
	 * order is generally wrong because it may lead to a deadlock under
	 * certain circumstances.
	 */
	async_wait_for(req, &retval);
	loc_exchange_end(exch);

	if (retval != EOK) {
		if (sid != NULL)
			*sid = -1;

		return retval;
	}

	if (sid != NULL)
		*sid = (service_id_t) IPC_GET_ARG1(answer);

	return retval;
}

/** Unregister service.
 *
 * @param sid	Service ID
 */
errno_t loc_service_unregister(service_id_t sid)
{
	async_exch_t *exch;
	errno_t retval;

	exch = loc_exchange_begin_blocking(INTERFACE_LOC_SUPPLIER);
	retval = async_req_1_0(exch, LOC_SERVICE_UNREGISTER, sid);
	loc_exchange_end(exch);

	return (errno_t)retval;
}

errno_t loc_service_get_id(const char *fqdn, service_id_t *handle,
    unsigned int flags)
{
	async_exch_t *exch;

	if (flags & IPC_FLAG_BLOCKING)
		exch = loc_exchange_begin_blocking(INTERFACE_LOC_CONSUMER);
	else {
		exch = loc_exchange_begin(INTERFACE_LOC_CONSUMER);
		if (exch == NULL)
			return errno;
	}

	ipc_call_t answer;
	aid_t req = async_send_2(exch, LOC_SERVICE_GET_ID, flags, 0,
	    &answer);
	errno_t retval = async_data_write_start(exch, fqdn, str_size(fqdn));

	loc_exchange_end(exch);

	if (retval != EOK) {
		async_forget(req);
		return retval;
	}

	async_wait_for(req, &retval);

	if (retval != EOK) {
		if (handle != NULL)
			*handle = (service_id_t) -1;

		return retval;
	}

	if (handle != NULL)
		*handle = (service_id_t) IPC_GET_ARG1(answer);

	return retval;
}

/** Get object name.
 *
 * Provided ID of an object, return its name.
 *
 * @param method	IPC method
 * @param id		Object ID
 * @param name		Place to store pointer to new string. Caller should
 *			free it using free().
 * @return		EOK on success or an error code
 */
static errno_t loc_get_name_internal(sysarg_t method, sysarg_t id, char **name)
{
	async_exch_t *exch;
	char name_buf[LOC_NAME_MAXLEN + 1];
	ipc_call_t dreply;
	size_t act_size;
	errno_t dretval;

	*name = NULL;
	exch = loc_exchange_begin_blocking(INTERFACE_LOC_CONSUMER);

	ipc_call_t answer;
	aid_t req = async_send_1(exch, method, id, &answer);
	aid_t dreq = async_data_read(exch, name_buf, LOC_NAME_MAXLEN,
	    &dreply);
	async_wait_for(dreq, &dretval);

	loc_exchange_end(exch);

	if (dretval != EOK) {
		async_forget(req);
		return dretval;
	}

	errno_t retval;
	async_wait_for(req, &retval);

	if (retval != EOK)
		return retval;

	act_size = IPC_GET_ARG2(dreply);
	assert(act_size <= LOC_NAME_MAXLEN);
	name_buf[act_size] = '\0';

	*name = str_dup(name_buf);
	if (*name == NULL)
		return ENOMEM;

	return EOK;
}

/** Get category name.
 *
 * Provided ID of a service, return its name.
 *
 * @param cat_id	Category ID
 * @param name		Place to store pointer to new string. Caller should
 *			free it using free().
 * @return		EOK on success or an error code
 */
errno_t loc_category_get_name(category_id_t cat_id, char **name)
{
	return loc_get_name_internal(LOC_CATEGORY_GET_NAME, cat_id, name);
}

/** Get service name.
 *
 * Provided ID of a service, return its name.
 *
 * @param svc_id	Service ID
 * @param name		Place to store pointer to new string. Caller should
 *			free it using free().
 * @return		EOK on success or an error code
 */
errno_t loc_service_get_name(service_id_t svc_id, char **name)
{
	return loc_get_name_internal(LOC_SERVICE_GET_NAME, svc_id, name);
}

/** Get service server name.
 *
 * Provided ID of a service, return the name of its server.
 *
 * @param svc_id	Service ID
 * @param name		Place to store pointer to new string. Caller should
 *			free it using free().
 * @return		EOK on success or an error code
 */
errno_t loc_service_get_server_name(service_id_t svc_id, char **name)
{
	return loc_get_name_internal(LOC_SERVICE_GET_SERVER_NAME, svc_id, name);
}

errno_t loc_namespace_get_id(const char *name, service_id_t *handle,
    unsigned int flags)
{
	async_exch_t *exch;

	if (flags & IPC_FLAG_BLOCKING)
		exch = loc_exchange_begin_blocking(INTERFACE_LOC_CONSUMER);
	else {
		exch = loc_exchange_begin(INTERFACE_LOC_CONSUMER);
		if (exch == NULL)
			return errno;
	}

	ipc_call_t answer;
	aid_t req = async_send_2(exch, LOC_NAMESPACE_GET_ID, flags, 0,
	    &answer);
	errno_t retval = async_data_write_start(exch, name, str_size(name));

	loc_exchange_end(exch);

	if (retval != EOK) {
		async_forget(req);
		return retval;
	}

	async_wait_for(req, &retval);

	if (retval != EOK) {
		if (handle != NULL)
			*handle = (service_id_t) -1;

		return retval;
	}

	if (handle != NULL)
		*handle = (service_id_t) IPC_GET_ARG1(answer);

	return retval;
}

/** Get category ID.
 *
 * Provided name of a category, return its ID.
 *
 * @param name		Category name
 * @param cat_id	Place to store ID
 * @param flags		IPC_FLAG_BLOCKING to wait for location service to start
 * @return		EOK on success or an error code
 */
errno_t loc_category_get_id(const char *name, category_id_t *cat_id,
    unsigned int flags)
{
	async_exch_t *exch;

	if (flags & IPC_FLAG_BLOCKING)
		exch = loc_exchange_begin_blocking(INTERFACE_LOC_CONSUMER);
	else {
		exch = loc_exchange_begin(INTERFACE_LOC_CONSUMER);
		if (exch == NULL)
			return errno;
	}

	ipc_call_t answer;
	aid_t req = async_send_0(exch, LOC_CATEGORY_GET_ID,
	    &answer);
	errno_t retval = async_data_write_start(exch, name, str_size(name));

	loc_exchange_end(exch);

	if (retval != EOK) {
		async_forget(req);
		return retval;
	}

	async_wait_for(req, &retval);

	if (retval != EOK) {
		if (cat_id != NULL)
			*cat_id = (category_id_t) -1;

		return retval;
	}

	if (cat_id != NULL)
		*cat_id = (category_id_t) IPC_GET_ARG1(answer);

	return retval;
}


loc_object_type_t loc_id_probe(service_id_t handle)
{
	async_exch_t *exch = loc_exchange_begin_blocking(INTERFACE_LOC_CONSUMER);

	sysarg_t type;
	errno_t retval = async_req_1_1(exch, LOC_ID_PROBE, handle, &type);

	loc_exchange_end(exch);

	if (retval != EOK)
		return LOC_OBJECT_NONE;

	return (loc_object_type_t) type;
}

async_sess_t *loc_service_connect(service_id_t handle, iface_t iface,
    unsigned int flags)
{
	async_sess_t *sess;

	if (flags & IPC_FLAG_BLOCKING)
		sess = service_connect_blocking(SERVICE_LOC, iface, handle);
	else
		sess = service_connect(SERVICE_LOC, iface, handle);

	return sess;
}

/**
 * @return ID of a new NULL device, or -1 if failed.
 */
int loc_null_create(void)
{
	async_exch_t *exch = loc_exchange_begin_blocking(INTERFACE_LOC_CONSUMER);

	sysarg_t null_id;
	errno_t retval = async_req_0_1(exch, LOC_NULL_CREATE, &null_id);

	loc_exchange_end(exch);

	if (retval != EOK)
		return -1;

	return (int) null_id;
}

void loc_null_destroy(int null_id)
{
	async_exch_t *exch = loc_exchange_begin_blocking(INTERFACE_LOC_CONSUMER);
	async_req_1_0(exch, LOC_NULL_DESTROY, (sysarg_t) null_id);
	loc_exchange_end(exch);
}

static size_t loc_count_namespaces_internal(async_exch_t *exch)
{
	sysarg_t count;
	errno_t retval = async_req_0_1(exch, LOC_GET_NAMESPACE_COUNT, &count);
	if (retval != EOK)
		return 0;

	return count;
}

/** Add service to category.
 *
 * @param svc_id	Service ID
 * @param cat_id	Category ID
 * @return		EOK on success or an error code
 */
errno_t loc_service_add_to_cat(service_id_t svc_id, service_id_t cat_id)
{
	async_exch_t *exch;
	errno_t retval;

	exch = loc_exchange_begin_blocking(INTERFACE_LOC_SUPPLIER);
	retval = async_req_2_0(exch, LOC_SERVICE_ADD_TO_CAT, svc_id, cat_id);
	loc_exchange_end(exch);

	return retval;
}

static size_t loc_count_services_internal(async_exch_t *exch,
    service_id_t ns_handle)
{
	sysarg_t count;
	errno_t retval = async_req_1_1(exch, LOC_GET_SERVICE_COUNT, ns_handle,
	    &count);
	if (retval != EOK)
		return 0;

	return count;
}

size_t loc_count_namespaces(void)
{
	async_exch_t *exch = loc_exchange_begin_blocking(INTERFACE_LOC_CONSUMER);
	size_t size = loc_count_namespaces_internal(exch);
	loc_exchange_end(exch);

	return size;
}

size_t loc_count_services(service_id_t ns_handle)
{
	async_exch_t *exch = loc_exchange_begin_blocking(INTERFACE_LOC_CONSUMER);
	size_t size = loc_count_services_internal(exch, ns_handle);
	loc_exchange_end(exch);

	return size;
}

size_t loc_get_namespaces(loc_sdesc_t **data)
{
	/* Loop until read is succesful */
	while (true) {
		async_exch_t *exch = loc_exchange_begin_blocking(INTERFACE_LOC_CONSUMER);
		size_t count = loc_count_namespaces_internal(exch);
		loc_exchange_end(exch);

		if (count == 0)
			return 0;

		loc_sdesc_t *devs = (loc_sdesc_t *) calloc(count, sizeof(loc_sdesc_t));
		if (devs == NULL)
			return 0;

		exch = loc_exchange_begin(INTERFACE_LOC_CONSUMER);

		ipc_call_t answer;
		aid_t req = async_send_0(exch, LOC_GET_NAMESPACES, &answer);
		errno_t rc = async_data_read_start(exch, devs, count * sizeof(loc_sdesc_t));

		loc_exchange_end(exch);

		if (rc == EOVERFLOW) {
			/*
			 * Number of namespaces has changed since
			 * the last call of LOC_GET_NAMESPACE_COUNT
			 */
			free(devs);
			continue;
		}

		if (rc != EOK) {
			async_forget(req);
			free(devs);
			return 0;
		}

		errno_t retval;
		async_wait_for(req, &retval);

		if (retval != EOK)
			return 0;

		*data = devs;
		return count;
	}
}

size_t loc_get_services(service_id_t ns_handle, loc_sdesc_t **data)
{
	/* Loop until read is succesful */
	while (true) {
		async_exch_t *exch = loc_exchange_begin_blocking(INTERFACE_LOC_CONSUMER);
		size_t count = loc_count_services_internal(exch, ns_handle);
		loc_exchange_end(exch);

		if (count == 0)
			return 0;

		loc_sdesc_t *devs = (loc_sdesc_t *) calloc(count, sizeof(loc_sdesc_t));
		if (devs == NULL)
			return 0;

		exch = loc_exchange_begin(INTERFACE_LOC_CONSUMER);

		ipc_call_t answer;
		aid_t req = async_send_1(exch, LOC_GET_SERVICES, ns_handle, &answer);
		errno_t rc = async_data_read_start(exch, devs, count * sizeof(loc_sdesc_t));

		loc_exchange_end(exch);

		if (rc == EOVERFLOW) {
			/*
			 * Number of services has changed since
			 * the last call of LOC_GET_SERVICE_COUNT
			 */
			free(devs);
			continue;
		}

		if (rc != EOK) {
			async_forget(req);
			free(devs);
			return 0;
		}

		errno_t retval;
		async_wait_for(req, &retval);

		if (retval != EOK)
			return 0;

		*data = devs;
		return count;
	}
}

static errno_t loc_category_get_ids_once(sysarg_t method, sysarg_t arg1,
    sysarg_t *id_buf, size_t buf_size, size_t *act_size)
{
	async_exch_t *exch = loc_exchange_begin_blocking(INTERFACE_LOC_CONSUMER);

	ipc_call_t answer;
	aid_t req = async_send_1(exch, method, arg1, &answer);
	errno_t rc = async_data_read_start(exch, id_buf, buf_size);

	loc_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);

	if (retval != EOK) {
		return retval;
	}

	*act_size = IPC_GET_ARG1(answer);
	return EOK;
}

/** Get list of IDs.
 *
 * Returns an allocated array of service IDs.
 *
 * @param method	IPC method
 * @param arg1		IPC argument 1
 * @param data		Place to store pointer to array of IDs
 * @param count		Place to store number of IDs
 * @return 		EOK on success or an error code
 */
static errno_t loc_get_ids_internal(sysarg_t method, sysarg_t arg1,
    sysarg_t **data, size_t *count)
{
	*data = NULL;
	*count = 0;

	size_t act_size = 0;
	errno_t rc = loc_category_get_ids_once(method, arg1, NULL, 0,
	    &act_size);
	if (rc != EOK)
		return rc;

	size_t alloc_size = act_size;
	service_id_t *ids = malloc(alloc_size);
	if (ids == NULL)
		return ENOMEM;

	while (true) {
		rc = loc_category_get_ids_once(method, arg1, ids, alloc_size,
		    &act_size);
		if (rc != EOK)
			return rc;

		if (act_size <= alloc_size)
			break;

		alloc_size = act_size;
		ids = realloc(ids, alloc_size);
		if (ids == NULL)
			return ENOMEM;
	}

	*count = act_size / sizeof(category_id_t);
	*data = ids;
	return EOK;
}

/** Get list of services in category.
 *
 * Returns an allocated array of service IDs.
 *
 * @param cat_id	Category ID
 * @param data		Place to store pointer to array of IDs
 * @param count		Place to store number of IDs
 * @return 		EOK on success or an error code
 */
errno_t loc_category_get_svcs(category_id_t cat_id, service_id_t **data,
    size_t *count)
{
	return loc_get_ids_internal(LOC_CATEGORY_GET_SVCS, cat_id,
	    data, count);
}

/** Get list of categories.
 *
 * Returns an allocated array of category IDs.
 *
 * @param data		Place to store pointer to array of IDs
 * @param count		Place to store number of IDs
 * @return 		EOK on success or an error code
 */
errno_t loc_get_categories(category_id_t **data, size_t *count)
{
	return loc_get_ids_internal(LOC_GET_CATEGORIES, 0,
	    data, count);
}

errno_t loc_register_cat_change_cb(loc_cat_change_cb_t cb_fun)
{
	fibril_mutex_lock(&loc_callback_mutex);
	if (loc_callback_create() != EOK) {
		fibril_mutex_unlock(&loc_callback_mutex);
		return EIO;
	}

	cat_change_cb = cb_fun;
	fibril_mutex_unlock(&loc_callback_mutex);

	return EOK;
}
