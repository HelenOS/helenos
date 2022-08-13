/*
 * SPDX-FileCopyrightText: 2009 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_LOC_H_
#define _LIBC_LOC_H_

#include <ipc/loc.h>
#include <async.h>
#include <stdbool.h>

typedef void (*loc_cat_change_cb_t)(void *);

extern async_exch_t *loc_exchange_begin_blocking(iface_t);
extern async_exch_t *loc_exchange_begin(iface_t);
extern void loc_exchange_end(async_exch_t *);

extern errno_t loc_server_register(const char *);
extern errno_t loc_service_register(const char *, service_id_t *);
extern errno_t loc_service_unregister(service_id_t);
extern errno_t loc_service_add_to_cat(service_id_t, category_id_t);

extern errno_t loc_service_get_id(const char *, service_id_t *,
    unsigned int);
extern errno_t loc_service_get_name(service_id_t, char **);
extern errno_t loc_service_get_server_name(service_id_t, char **);
extern errno_t loc_namespace_get_id(const char *, service_id_t *,
    unsigned int);
extern errno_t loc_category_get_id(const char *, category_id_t *,
    unsigned int);
extern errno_t loc_category_get_name(category_id_t, char **);
extern errno_t loc_category_get_svcs(category_id_t, category_id_t **, size_t *);
extern loc_object_type_t loc_id_probe(service_id_t);

extern async_sess_t *loc_service_connect(service_id_t, iface_t,
    unsigned int);

extern int loc_null_create(void);
extern void loc_null_destroy(int);

extern size_t loc_count_namespaces(void);
extern size_t loc_count_services(service_id_t);

extern size_t loc_get_namespaces(loc_sdesc_t **);
extern size_t loc_get_services(service_id_t, loc_sdesc_t **);
extern errno_t loc_get_categories(category_id_t **, size_t *);
extern errno_t loc_register_cat_change_cb(loc_cat_change_cb_t, void *);

#endif

/** @}
 */
