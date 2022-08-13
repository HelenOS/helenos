/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup locsrv
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
