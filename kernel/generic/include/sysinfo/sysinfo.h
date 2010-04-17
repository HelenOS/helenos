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

#ifndef KERN_SYSINFO_H_
#define KERN_SYSINFO_H_

#include <typedefs.h>
#include <str.h>

extern bool fb_exported;

typedef enum {
	SYSINFO_VAL_UNDEFINED = 0,
	SYSINFO_VAL_VAL = 1,
	SYSINFO_VAL_DATA = 2,
	SYSINFO_VAL_FUNCTION_VAL = 3,
	SYSINFO_VAL_FUNCTION_DATA = 4
} sysinfo_item_val_type_t;

typedef enum {
	SYSINFO_SUBTREE_NONE = 0,
	SYSINFO_SUBTREE_TABLE = 1,
	SYSINFO_SUBTREE_FUNCTION = 2
} sysinfo_subtree_type_t;

struct sysinfo_item;

typedef unative_t (*sysinfo_fn_val_t)(struct sysinfo_item *);
typedef void *(*sysinfo_fn_data_t)(struct sysinfo_item *, size_t *);
typedef struct sysinfo_item *(*sysinfo_fn_subtree_t)(const char *);

typedef struct {
	void *data;
	size_t size;
} sysinfo_data_t;

typedef union {
	unative_t val;
	sysinfo_fn_val_t fn_val;
	sysinfo_fn_data_t fn_data;
	sysinfo_data_t data;
} sysinfo_item_val_t;

typedef union {
	struct sysinfo_item *table;
	sysinfo_fn_subtree_t find_item;
} sysinfo_subtree_t;

typedef struct sysinfo_item {
	char *name;
	
	sysinfo_item_val_type_t val_type;
	sysinfo_item_val_t val;
	
	sysinfo_subtree_type_t subtree_type;
	sysinfo_subtree_t subtree;
	
	struct sysinfo_item *next;
} sysinfo_item_t;

typedef struct {
	sysinfo_item_val_type_t tag;
	union {
		unative_t val;
		sysinfo_data_t data;
	};
} sysinfo_return_t;

extern void sysinfo_init(void);

extern void sysinfo_set_item_val(const char *, sysinfo_item_t **, unative_t);
extern void sysinfo_set_item_data(const char *, sysinfo_item_t **, void *,
    size_t);
extern void sysinfo_set_item_val_fn(const char *, sysinfo_item_t **,
    sysinfo_fn_val_t);
extern void sysinfo_set_item_data_fn(const char *, sysinfo_item_t **,
    sysinfo_fn_data_t);
extern void sysinfo_set_item_undefined(const char *, sysinfo_item_t **);

extern sysinfo_return_t sysinfo_get_item(const char *, sysinfo_item_t **);
extern void sysinfo_dump(sysinfo_item_t **, unsigned int);

unative_t sys_sysinfo_get_tag(void *, size_t);
unative_t sys_sysinfo_get_value(void *, size_t, void *);
unative_t sys_sysinfo_get_data_size(void *, size_t, void *);
unative_t sys_sysinfo_get_data(void *, size_t, void *, size_t);

#endif

/** @}
 */
