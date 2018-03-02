/*
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

/** @addtogroup loc
 * @{
 */
/** @file Categories for location service.
 */

#ifndef CATEGORY_H_
#define CATEGORY_H_

#include <adt/list.h>
#include "locsrv.h"

typedef sysarg_t catid_t;

/** Service category */
typedef struct {
	/** Protects this structure, list of services */
	fibril_mutex_t mutex;

	/** Identifier */
	catid_t id;

	/** Category name */
	const char *name;

	/** Link to list of categories (categ_dir_t.categories) */
	link_t cat_list;

	/** List of service memberships in this category (svc_categ_t) */
	list_t svc_memb;
} category_t;

/** Service directory ogranized by categories (yellow pages) */
typedef struct {
	/** Protects this structure, list of categories */
	fibril_mutex_t mutex;
	/** List of all categories (category_t) */
	list_t categories;
} categ_dir_t;

/** Service in category membership. */
typedef struct {
	/** Link to category_t.svc_memb list */
	link_t cat_link;
	/** Link to loc_service_t.cat_memb list */
	link_t svc_link;

	/** Category */
	category_t *cat;
	/** Service */
	loc_service_t *svc;
} svc_categ_t;

extern void categ_dir_init(categ_dir_t *);
extern void categ_dir_add_cat(categ_dir_t *, category_t *);
extern errno_t categ_dir_get_categories(categ_dir_t *, service_id_t *, size_t,
    size_t *);
extern category_t *category_new(const char *);
extern errno_t category_add_service(category_t *, loc_service_t *);
extern void category_remove_service(svc_categ_t *);
extern category_t *category_get(categ_dir_t *, catid_t);
extern category_t *category_find_by_name(categ_dir_t *, const char *);
extern errno_t category_get_services(category_t *, service_id_t *, size_t,
    size_t *);


#endif

/** @}
 */
