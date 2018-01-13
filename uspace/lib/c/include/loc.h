/*
 * Copyright (c) 2009 Jiri Svoboda
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
/** @file
 */

#ifndef LIBC_LOC_H_
#define LIBC_LOC_H_

#include <ipc/loc.h>
#include <async.h>
#include <stdbool.h>

typedef void (*loc_cat_change_cb_t)(void);

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
extern errno_t loc_register_cat_change_cb(loc_cat_change_cb_t);


#endif

/** @}
 */
