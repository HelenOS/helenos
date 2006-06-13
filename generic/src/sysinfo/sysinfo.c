/*
 * Copyright (C) 2006 Jakub Vana
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

sysinfo_item_t *_root = NULL;


static sysinfo_item_t *sysinfo_find_item(const char *name, sysinfo_item_t *subtree)
{
	if (subtree == NULL)
		return NULL;
	
	while (subtree != NULL)	{
		int i = 0;
		char *a = (char *) name;
		char *b = subtree->name;
		
		while ((a[i] == b[i]) && (b[i]))
			i++;
		
		if ((!a[i]) && (!b[i]))  /* Last name in path matches */
			return subtree;
		
		if ((a[i] == '.') && (!b[i])) { /* Middle name in path matches */
			if (subtree->subinfo_type == SYSINFO_SUBINFO_TABLE)
				return sysinfo_find_item(a + i + 1, subtree->subinfo.table);
			
			//if (subtree->subinfo_type == SYSINFO_SUBINFO_FUNCTION) /* Subinfo managed by subsystem */
			//	return NULL; 
			
			return NULL; /* No subinfo */
		}
		/* No matches try next */
		subtree = subtree->next;
		i = 0;
	}
	return NULL;
}

static sysinfo_item_t *sysinfo_create_path(const char *name, sysinfo_item_t **psubtree)
{
	sysinfo_item_t *subtree;
	subtree = *psubtree;
	
	if (subtree == NULL) {
			sysinfo_item_t *item = malloc(sizeof(sysinfo_item_t), 0);
			int i = 0, j;
			
			ASSERT(item);
			*psubtree = item;
			item->next = NULL;
			item->val_type = SYSINFO_VAL_UNDEFINED;
			item->subinfo.table = NULL;

			while (name[i] && (name[i] != '.'))
				i++;
			
			item->name = malloc(i, 0);
			ASSERT(item->name);

			for (j = 0; j < i; j++)
				item->name[j] = name[j];
			item->name[j] = 0;
			
			if (name[i]) { /* =='.' */
				item->subinfo_type = SYSINFO_SUBINFO_TABLE;
				return sysinfo_create_path(name + i + 1, &(item->subinfo.table));
			}
			item->subinfo_type = SYSINFO_SUBINFO_NONE;
			return item;
	}

	while (subtree != NULL) {
		int i = 0, j;
		char *a = (char *) name;
		char *b = subtree->name;
		
		while ((a[i] == b[i]) && (b[i]))
			i++;
		
		if ((!a[i]) && (!b[i])) /* Last name in path matches */
			return subtree;
		
		if ((a[i] == '.') && (!b[i])) { /* Middle name in path matches */
			if (subtree->subinfo_type == SYSINFO_SUBINFO_TABLE)
				return sysinfo_create_path(a + i + 1, &(subtree->subinfo.table));
			
			if (subtree->subinfo_type == SYSINFO_SUBINFO_NONE) {
				subtree->subinfo_type = SYSINFO_SUBINFO_TABLE;
				return sysinfo_create_path(a + i + 1,&(subtree->subinfo.table));
			}
			
			//if (subtree->subinfo_type == SYSINFO_SUBINFO_FUNCTION) /* Subinfo managed by subsystem */
			//	return NULL; 
			
			return NULL;
		}
		/* No matches try next or create new*/
		if (subtree->next == NULL) {
			sysinfo_item_t *item = malloc(sizeof(sysinfo_item_t), 0);
			
			ASSERT(item);
			subtree->next = item;
			item->next = NULL;
			item->val_type = SYSINFO_VAL_UNDEFINED;
			item->subinfo.table = NULL;

			i = 0;
			while (name[i] && (name[i] != '.'))
				i++;

			item->name = malloc(i, 0);
			ASSERT(item->name);
			
			for (j = 0; j < i; j++)
				item->name[j] = name[j];
			
			item->name[j] = 0;

			if(name[i]) { /* =='.' */
				item->subinfo_type = SYSINFO_SUBINFO_TABLE;
				return sysinfo_create_path(name + i + 1, &(item->subinfo.table));
			}
			item->subinfo_type = SYSINFO_SUBINFO_NONE;
			return item;
		} else {
			subtree = subtree->next;
			i = 0;
		}	
	}
	panic("Not reached\n");
	return NULL;
}

void sysinfo_set_item_val(const char *name, sysinfo_item_t **root, __native val)
{
	if (root == NULL)
		root = &_root;
	
	/* If already created create only returns pointer 
	   If not, create it */
	sysinfo_item_t *item = sysinfo_create_path(name, root);
	
	if (item != NULL) { /* If in subsystem, unable to create or return so unable to set */
		item->val.val=val;                   
		item->val_type = SYSINFO_VAL_VAL;
	}
}

void sysinfo_set_item_function(const char *name, sysinfo_item_t **root, sysinfo_val_fn_t fn)
{
	if (root == NULL)
		root = &_root;
	
	/* If already created create only returns pointer 
	   If not, create it */
	sysinfo_item_t *item = sysinfo_create_path(name, root);
	
	if (item != NULL) { /* If in subsystem, unable to create or return so  unable to set */
		item->val.fn=fn;                   
		item->val_type = SYSINFO_VAL_FUNCTION;
	}
}


void sysinfo_set_item_undefined(const char *name, sysinfo_item_t **root)
{
	if (root == NULL)
		root = &_root;
	
	/* If already created create only returns pointer 
	   If not, create it */
	sysinfo_item_t *item = sysinfo_create_path(name, root);
	
	if (item != NULL)
		item->val_type = SYSINFO_VAL_UNDEFINED;
}


void sysinfo_dump(sysinfo_item_t **proot, int depth)
{
	sysinfo_item_t *root;
	if (proot == NULL)
		proot = &_root;
	
	root = *proot;
	
	while (root != NULL) {
		int i;
		__native val = 0;
		char *vtype = NULL;
		
		
		for (i = 0; i < depth; i++)
			printf("  ");
		
		switch (root->val_type) {
			case SYSINFO_VAL_UNDEFINED:
				val = 0;
				vtype = "UND";
				break;
			case SYSINFO_VAL_VAL:
				val = root->val.val;
				vtype = "VAL";
				break;
			case SYSINFO_VAL_FUNCTION:
				val = ((sysinfo_val_fn_t) (root->val.fn)) (root);
				vtype = "FUN";
				break;
		}
		
		printf("%s    %s val:%d(%X) sub:%s\n", root->name, vtype, val, val, (root->subinfo_type == SYSINFO_SUBINFO_NONE) ? "NON" : ((root->subinfo_type == SYSINFO_SUBINFO_TABLE) ? "TAB" : "FUN"));
		
		if (root->subinfo_type == SYSINFO_SUBINFO_TABLE)
			sysinfo_dump(&(root -> subinfo.table), depth + 1);
		
		root = root->next;
	}
}

sysinfo_rettype_t sysinfo_get_val(const char *name, sysinfo_item_t **root)
{
	// TODO: Implement Subsystem subinfo (by function implemented subinfo)

	sysinfo_rettype_t ret = {0, false};

	if (root == NULL)
		root = &_root;
	
	sysinfo_item_t *item = sysinfo_find_item(name, *root);
	
	if (item != NULL) {
		if (item->val_type == SYSINFO_VAL_UNDEFINED) 
			return ret;
		else
			ret.valid = true;
		
		if (item->val_type == SYSINFO_VAL_VAL)
			ret.val = item->val.val;
		else
			ret.val = ((sysinfo_val_fn_t) (item->val.fn)) (item);
	}
	return ret;
}

__native sys_sysinfo_valid(__native ptr, __native len)
{
	char *str;
	sysinfo_rettype_t ret = {0, 0};
	str = malloc(len + 1, 0);
	
	ASSERT(str);
	if (!((copy_from_uspace(str, (void *) ptr, len + 1)) || (str[len])))
		ret = sysinfo_get_val(str, NULL);
	
	free(str);
	return ret.valid;
}

__native sys_sysinfo_value(__native ptr, __native len)
{
	char *str;
	sysinfo_rettype_t ret = {0, 0};
	str = malloc(len + 1, 0);
	
	ASSERT(str);
	if (!((copy_from_uspace(str, (void *) ptr, len + 1)) || (str[len]))) 
		ret = sysinfo_get_val(str, NULL);
	
	free(str);
	return ret.val;
}

 /** @}
 */

