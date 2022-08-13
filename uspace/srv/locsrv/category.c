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

#include <adt/list.h>
#include <errno.h>
#include <fibril_synch.h>
#include <stdlib.h>
#include <str.h>

#include "category.h"
#include "locsrv.h"

/** Initialize category directory. */
void categ_dir_init(categ_dir_t *cdir)
{
	fibril_mutex_initialize(&cdir->mutex);
	list_initialize(&cdir->categories);
}

/** Add new category to directory. */
void categ_dir_add_cat(categ_dir_t *cdir, category_t *cat)
{
	list_append(&cat->cat_list, &cdir->categories);
}

/** Get list of categories. */
errno_t categ_dir_get_categories(categ_dir_t *cdir, category_id_t *id_buf,
    size_t buf_size, size_t *act_size)
{
	size_t act_cnt;
	size_t buf_cnt;

	assert(fibril_mutex_is_locked(&cdir->mutex));

	buf_cnt = buf_size / sizeof(category_id_t);

	act_cnt = list_count(&cdir->categories);
	*act_size = act_cnt * sizeof(category_id_t);

	if (buf_size % sizeof(category_id_t) != 0)
		return EINVAL;

	size_t pos = 0;
	list_foreach(cdir->categories, cat_list, category_t, cat) {
		if (pos < buf_cnt)
			id_buf[pos] = cat->id;
		pos++;
	}

	return EOK;
}

/** Initialize category structure. */
static void category_init(category_t *cat, const char *name)
{
	fibril_mutex_initialize(&cat->mutex);
	cat->name = str_dup(name);
	cat->id = loc_create_id();
	link_initialize(&cat->cat_list);
	list_initialize(&cat->svc_memb);
}

/** Allocate new category. */
category_t *category_new(const char *name)
{
	category_t *cat;

	cat = malloc(sizeof(category_t));
	if (cat == NULL)
		return NULL;

	category_init(cat, name);
	return cat;
}

/** Add service to category. */
errno_t category_add_service(category_t *cat, loc_service_t *svc)
{
	assert(fibril_mutex_is_locked(&cat->mutex));
	assert(fibril_mutex_is_locked(&services_list_mutex));

	/* Verify that category does not contain this service yet. */
	list_foreach(cat->svc_memb, cat_link, svc_categ_t, memb) {
		if (memb->svc == svc) {
			return EEXIST;
		}
	}

	svc_categ_t *nmemb = malloc(sizeof(svc_categ_t));
	if (nmemb == NULL)
		return ENOMEM;

	nmemb->svc = svc;
	nmemb->cat = cat;

	list_append(&nmemb->cat_link, &cat->svc_memb);
	list_append(&nmemb->svc_link, &svc->cat_memb);

	return EOK;
}

/** Remove service from category. */
void category_remove_service(svc_categ_t *memb)
{
	assert(fibril_mutex_is_locked(&memb->cat->mutex));
	assert(fibril_mutex_is_locked(&services_list_mutex));

	list_remove(&memb->cat_link);
	list_remove(&memb->svc_link);

	free(memb);
}

/** Get category by ID. */
category_t *category_get(categ_dir_t *cdir, catid_t catid)
{
	assert(fibril_mutex_is_locked(&cdir->mutex));

	list_foreach(cdir->categories, cat_list, category_t, cat) {
		if (cat->id == catid)
			return cat;
	}

	return NULL;
}

/** Find category by name. */
category_t *category_find_by_name(categ_dir_t *cdir, const char *name)
{
	assert(fibril_mutex_is_locked(&cdir->mutex));

	list_foreach(cdir->categories, cat_list, category_t, cat) {
		if (str_cmp(cat->name, name) == 0)
			return cat;
	}

	return NULL;
}

/** Get list of services in category. */
errno_t category_get_services(category_t *cat, service_id_t *id_buf,
    size_t buf_size, size_t *act_size)
{
	size_t act_cnt;
	size_t buf_cnt;

	assert(fibril_mutex_is_locked(&cat->mutex));

	buf_cnt = buf_size / sizeof(service_id_t);

	act_cnt = list_count(&cat->svc_memb);
	*act_size = act_cnt * sizeof(service_id_t);

	if (buf_size % sizeof(service_id_t) != 0)
		return EINVAL;

	size_t pos = 0;
	list_foreach(cat->svc_memb, cat_link, svc_categ_t, memb) {
		if (pos < buf_cnt)
			id_buf[pos] = memb->svc->id;
		pos++;
	}

	return EOK;
}

/**
 * @}
 */
