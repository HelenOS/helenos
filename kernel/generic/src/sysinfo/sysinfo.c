/*
 * Copyright (c) 2006 Jakub Vana
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

/** @addtogroup generic
 * @{
 */
/** @file
 */

#include <sysinfo/sysinfo.h>
#include <mm/slab.h>
#include <print.h>
#include <syscall/copy.h>
#include <synch/spinlock.h>
#include <errno.h>

#define SYSINFO_MAX_PATH  2048

bool fb_exported = false;

static sysinfo_item_t *global_root = NULL;
static slab_cache_t *sysinfo_item_slab;

SPINLOCK_STATIC_INITIALIZE_NAME(sysinfo_lock, "sysinfo_lock");

static int sysinfo_item_constructor(void *obj, int kmflag)
{
	sysinfo_item_t *item = (sysinfo_item_t *) obj;
	
	item->name = NULL;
	item->val_type = SYSINFO_VAL_UNDEFINED;
	item->subtree_type = SYSINFO_SUBTREE_NONE;
	item->subtree.table = NULL;
	item->next = NULL;
	
	return 0;
}

static int sysinfo_item_destructor(void *obj)
{
	sysinfo_item_t *item = (sysinfo_item_t *) obj;
	
	if (item->name != NULL)
		free(item->name);
	
	return 0;
}

void sysinfo_init(void)
{
	sysinfo_item_slab = slab_cache_create("sysinfo_item_slab",
	    sizeof(sysinfo_item_t), 0, sysinfo_item_constructor,
	    sysinfo_item_destructor, SLAB_CACHE_MAGDEFERRED);
}

/** Recursively find item in sysinfo tree
 *
 * Should be called with interrupts disabled
 * and sysinfo_lock held.
 *
 */
static sysinfo_item_t *sysinfo_find_item(const char *name,
    sysinfo_item_t *subtree, sysinfo_return_t **ret)
{
	ASSERT(subtree != NULL);
	ASSERT(ret != NULL);
	
	sysinfo_item_t *cur = subtree;
	
	while (cur != NULL) {
		size_t i = 0;
		
		/* Compare name with path */
		while ((cur->name[i] != 0) && (name[i] == cur->name[i]))
			i++;
		
		/* Check for perfect name and path match */
		if ((name[i] == 0) && (cur->name[i] == 0))
			return cur;
		
		/* Partial match up to the delimiter */
		if ((name[i] == '.') && (cur->name[i] == 0)) {
			/* Look into the subtree */
			switch (cur->subtree_type) {
			case SYSINFO_SUBTREE_TABLE:
				/* Recursively find in subtree */
				return sysinfo_find_item(name + i + 1,
				    cur->subtree.table, ret);
			case SYSINFO_SUBTREE_FUNCTION:
				/* Get generated data */
				**ret = cur->subtree.get_data(name + i + 1);
				return NULL;
			default:
				/* Not found */
				*ret = NULL;
				return NULL;
			}
		}
		
		cur = cur->next;
	}
	
	*ret = NULL;
	return NULL;
}

/** Recursively create items in sysinfo tree
 *
 * Should be called with interrupts disabled
 * and sysinfo_lock held.
 *
 */
static sysinfo_item_t *sysinfo_create_path(const char *name,
    sysinfo_item_t **psubtree)
{
	ASSERT(psubtree != NULL);
	
	if (*psubtree == NULL) {
		/* No parent */
		
		size_t i = 0;
		
		/* Find the first delimiter in name */
		while ((name[i] != 0) && (name[i] != '.'))
			i++;
		
		*psubtree =
		    (sysinfo_item_t *) slab_alloc(sysinfo_item_slab, 0);
		ASSERT(*psubtree);
		
		/* Fill in item name up to the delimiter */
		(*psubtree)->name = str_ndup(name, i);
		ASSERT((*psubtree)->name);
		
		/* Create subtree items */
		if (name[i] == '.') {
			(*psubtree)->subtree_type = SYSINFO_SUBTREE_TABLE;
			return sysinfo_create_path(name + i + 1,
			    &((*psubtree)->subtree.table));
		}
		
		/* No subtree needs to be created */
		return *psubtree;
	}
	
	sysinfo_item_t *cur = *psubtree;
	
	while (cur != NULL) {
		size_t i = 0;
		
		/* Compare name with path */
		while ((cur->name[i] != 0) && (name[i] == cur->name[i]))
			i++;
		
		/* Check for perfect name and path match
		 * -> item is already present.
		 */
		if ((name[i] == 0) && (cur->name[i] == 0))
			return cur;
		
		/* Partial match up to the delimiter */
		if ((name[i] == '.') && (cur->name[i] == 0)) {
			switch (cur->subtree_type) {
			case SYSINFO_SUBTREE_NONE:
				/* No subtree yet, create one */
				cur->subtree_type = SYSINFO_SUBTREE_TABLE;
				return sysinfo_create_path(name + i + 1,
				    &(cur->subtree.table));
			case SYSINFO_SUBTREE_TABLE:
				/* Subtree already created, add new sibling */
				return sysinfo_create_path(name + i + 1,
				    &(cur->subtree.table));
			default:
				/* Subtree items handled by a function, this
				 * cannot be overriden.
				 */
				return NULL;
			}
		}
		
		/* No match and no more siblings to check
		 * -> create a new sibling item.
		 */
		if (cur->next == NULL) {
			/* Find the first delimiter in name */
			i = 0;
			while ((name[i] != 0) && (name[i] != '.'))
				i++;
			
			sysinfo_item_t *item =
			    (sysinfo_item_t *) slab_alloc(sysinfo_item_slab, 0);
			ASSERT(item);
			
			cur->next = item;
			
			/* Fill in item name up to the delimiter */
			item->name = str_ndup(name, i);
			ASSERT(item->name);
			
			/* Create subtree items */
			if (name[i] == '.') {
				item->subtree_type = SYSINFO_SUBTREE_TABLE;
				return sysinfo_create_path(name + i + 1,
				    &(item->subtree.table));
			}
			
			/* No subtree needs to be created */
			return item;
		}
		
		/* Get next sibling */
		cur = cur->next;
	}
	
	/* Unreachable */
	ASSERT(false);
	return NULL;
}

void sysinfo_set_item_val(const char *name, sysinfo_item_t **root,
    unative_t val)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&sysinfo_lock);
	
	if (root == NULL)
		root = &global_root;
	
	sysinfo_item_t *item = sysinfo_create_path(name, root);
	if (item != NULL) {
		item->val_type = SYSINFO_VAL_VAL;
		item->val.val = val;
	}
	
	spinlock_unlock(&sysinfo_lock);
	interrupts_restore(ipl);
}

void sysinfo_set_item_data(const char *name, sysinfo_item_t **root,
    void *data, size_t size)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&sysinfo_lock);
	
	if (root == NULL)
		root = &global_root;
	
	sysinfo_item_t *item = sysinfo_create_path(name, root);
	if (item != NULL) {
		item->val_type = SYSINFO_VAL_DATA;
		item->val.data.data = data;
		item->val.data.size = size;
	}
	
	spinlock_unlock(&sysinfo_lock);
	interrupts_restore(ipl);
}

void sysinfo_set_item_fn_val(const char *name, sysinfo_item_t **root,
    sysinfo_fn_val_t fn)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&sysinfo_lock);
	
	if (root == NULL)
		root = &global_root;
	
	sysinfo_item_t *item = sysinfo_create_path(name, root);
	if (item != NULL) {
		item->val_type = SYSINFO_VAL_FUNCTION_VAL;
		item->val.fn_val = fn;
	}
	
	spinlock_unlock(&sysinfo_lock);
	interrupts_restore(ipl);
}

void sysinfo_set_item_fn_data(const char *name, sysinfo_item_t **root,
    sysinfo_fn_data_t fn)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&sysinfo_lock);
	
	if (root == NULL)
		root = &global_root;
	
	sysinfo_item_t *item = sysinfo_create_path(name, root);
	if (item != NULL) {
		item->val_type = SYSINFO_VAL_FUNCTION_DATA;
		item->val.fn_data = fn;
	}
	
	spinlock_unlock(&sysinfo_lock);
	interrupts_restore(ipl);
}

void sysinfo_set_item_undefined(const char *name, sysinfo_item_t **root)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&sysinfo_lock);
	
	if (root == NULL)
		root = &global_root;
	
	sysinfo_item_t *item = sysinfo_create_path(name, root);
	if (item != NULL)
		item->val_type = SYSINFO_VAL_UNDEFINED;
	
	spinlock_unlock(&sysinfo_lock);
	interrupts_restore(ipl);
}

void sysinfo_set_subtree_fn(const char *name, sysinfo_item_t **root,
    sysinfo_fn_subtree_t fn)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&sysinfo_lock);
	
	if (root == NULL)
		root = &global_root;
	
	sysinfo_item_t *item = sysinfo_create_path(name, root);
	if ((item != NULL) && (item->subtree_type != SYSINFO_SUBTREE_TABLE)) {
		item->subtree_type = SYSINFO_SUBTREE_FUNCTION;
		item->subtree.get_data = fn;
	}
	
	spinlock_unlock(&sysinfo_lock);
	interrupts_restore(ipl);
}

/** Sysinfo dump indentation helper routine
 *
 */
static void sysinfo_indent(unsigned int depth)
{
	unsigned int i;
	for (i = 0; i < depth; i++)
		printf("  ");
}

/** Dump the structure of sysinfo tree
 *
 * Should be called with interrupts disabled
 * and sysinfo_lock held. Because this routine
 * might take a reasonable long time to proceed,
 * having the spinlock held is not optimal, but
 * there is no better simple solution.
 *
 */
static void sysinfo_dump_internal(sysinfo_item_t *root, unsigned int depth)
{
	sysinfo_item_t *cur = root;
	
	while (cur != NULL) {
		sysinfo_indent(depth);
		
		unative_t val;
		void *data;
		size_t size;
		
		switch (cur->val_type) {
		case SYSINFO_VAL_UNDEFINED:
			printf("+ %s\n", cur->name);
			break;
		case SYSINFO_VAL_VAL:
			printf("+ %s -> %" PRIun" (%#" PRIxn ")\n", cur->name,
			    cur->val.val, cur->val.val);
			break;
		case SYSINFO_VAL_DATA:
			printf("+ %s (%" PRIs" bytes)\n", cur->name,
			    cur->val.data.size);
			break;
		case SYSINFO_VAL_FUNCTION_VAL:
			val = cur->val.fn_val(cur);
			printf("+ %s -> %" PRIun" (%#" PRIxn ") [generated]\n",
			    cur->name, val, val);
			break;
		case SYSINFO_VAL_FUNCTION_DATA:
			data = cur->val.fn_data(cur, &size);
			if (data != NULL)
				free(data);
			
			printf("+ %s (%" PRIs" bytes) [generated]\n", cur->name,
			    size);
			break;
		default:
			printf("+ %s [unknown]\n", cur->name);
		}
		
		switch (cur->subtree_type) {
		case SYSINFO_SUBTREE_NONE:
			break;
		case SYSINFO_SUBTREE_TABLE:
			sysinfo_dump_internal(cur->subtree.table, depth + 1);
			break;
		case SYSINFO_SUBTREE_FUNCTION:
			sysinfo_indent(depth + 1);
			printf("+ [generated subtree]\n");
			break;
		default:
			sysinfo_indent(depth + 1);
			printf("+ [unknown subtree]\n");
		}
		
		cur = cur->next;
	}
}

void sysinfo_dump(sysinfo_item_t *root)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&sysinfo_lock);
	
	if (root == NULL)
		sysinfo_dump_internal(global_root, 0);
	else
		sysinfo_dump_internal(root, 0);
	
	spinlock_unlock(&sysinfo_lock);
	interrupts_restore(ipl);
}

/** Return sysinfo item determined by name
 *
 * Should be called with interrupts disabled
 * and sysinfo_lock held.
 *
 */
static sysinfo_return_t sysinfo_get_item(const char *name, sysinfo_item_t **root)
{
	if (root == NULL)
		root = &global_root;
	
	sysinfo_return_t ret;
	sysinfo_return_t *ret_ptr = &ret;
	sysinfo_item_t *item = sysinfo_find_item(name, *root, &ret_ptr);
	
	if (item != NULL) {
		ret.tag = item->val_type;
		switch (item->val_type) {
		case SYSINFO_VAL_UNDEFINED:
			break;
		case SYSINFO_VAL_VAL:
			ret.val = item->val.val;
			break;
		case SYSINFO_VAL_DATA:
			ret.data = item->val.data;
			break;
		case SYSINFO_VAL_FUNCTION_VAL:
			ret.val = item->val.fn_val(item);
			break;
		case SYSINFO_VAL_FUNCTION_DATA:
			ret.data.data = item->val.fn_data(item, &ret.data.size);
			break;
		}
	} else {
		if (ret_ptr == NULL)
			ret.tag = SYSINFO_VAL_UNDEFINED;
	}
	
	return ret;
}

/** Return sysinfo item determined by name from user space
 *
 * Should be called with interrupts disabled
 * and sysinfo_lock held.
 *
 */
static sysinfo_return_t sysinfo_get_item_uspace(void *ptr, size_t size)
{
	sysinfo_return_t ret;
	ret.tag = SYSINFO_VAL_UNDEFINED;
	
	if (size > SYSINFO_MAX_PATH)
		return ret;
	
	char *path = (char *) malloc(size + 1, 0);
	ASSERT(path);
	
	if ((copy_from_uspace(path, ptr, size + 1) == 0)
	    && (path[size] == 0))
		ret = sysinfo_get_item(path, NULL);
	
	free(path);
	return ret;
}

unative_t sys_sysinfo_get_tag(void *path_ptr, size_t path_size)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&sysinfo_lock);
	
	sysinfo_return_t ret = sysinfo_get_item_uspace(path_ptr, path_size);
	
	if ((ret.tag == SYSINFO_VAL_FUNCTION_DATA) && (ret.data.data != NULL))
		free(ret.data.data);
	
	if (ret.tag == SYSINFO_VAL_FUNCTION_VAL)
		ret.tag = SYSINFO_VAL_VAL;
	else if (ret.tag == SYSINFO_VAL_FUNCTION_DATA)
		ret.tag = SYSINFO_VAL_DATA;
	
	spinlock_unlock(&sysinfo_lock);
	interrupts_restore(ipl);
	
	return (unative_t) ret.tag;
}

unative_t sys_sysinfo_get_value(void *path_ptr, size_t path_size,
    void *value_ptr)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&sysinfo_lock);
	
	sysinfo_return_t ret = sysinfo_get_item_uspace(path_ptr, path_size);
	int rc;
	
	if ((ret.tag == SYSINFO_VAL_VAL) || (ret.tag == SYSINFO_VAL_FUNCTION_VAL))
		rc = copy_to_uspace(value_ptr, &ret.val, sizeof(ret.val));
	else
		rc = EINVAL;
	
	if ((ret.tag == SYSINFO_VAL_FUNCTION_DATA) && (ret.data.data != NULL))
		free(ret.data.data);
	
	spinlock_unlock(&sysinfo_lock);
	interrupts_restore(ipl);
	
	return (unative_t) rc;
}

unative_t sys_sysinfo_get_data_size(void *path_ptr, size_t path_size,
    void *size_ptr)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&sysinfo_lock);
	
	sysinfo_return_t ret = sysinfo_get_item_uspace(path_ptr, path_size);
	int rc;
	
	if ((ret.tag == SYSINFO_VAL_DATA) || (ret.tag == SYSINFO_VAL_FUNCTION_DATA))
		rc = copy_to_uspace(size_ptr, &ret.data.size,
		    sizeof(ret.data.size));
	else
		rc = EINVAL;
	
	if ((ret.tag == SYSINFO_VAL_FUNCTION_DATA) && (ret.data.data != NULL))
		free(ret.data.data);
	
	spinlock_unlock(&sysinfo_lock);
	interrupts_restore(ipl);
	
	return (unative_t) rc;
}

unative_t sys_sysinfo_get_data(void *path_ptr, size_t path_size,
    void *buffer_ptr, size_t buffer_size)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&sysinfo_lock);
	
	sysinfo_return_t ret = sysinfo_get_item_uspace(path_ptr, path_size);
	int rc;
	
	if ((ret.tag == SYSINFO_VAL_DATA) || (ret.tag == SYSINFO_VAL_FUNCTION_DATA)) {
		if (ret.data.size == buffer_size)
			rc = copy_to_uspace(buffer_ptr, ret.data.data,
			    ret.data.size);
		else
			rc = ENOMEM;
	} else
		rc = EINVAL;
	
	if ((ret.tag == SYSINFO_VAL_FUNCTION_DATA) && (ret.data.data != NULL))
		free(ret.data.data);
	
	spinlock_unlock(&sysinfo_lock);
	interrupts_restore(ipl);
	
	return (unative_t) rc;
}

/** @}
 */
