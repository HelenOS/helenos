/*
 * Copyright (c) 2006 Jakub Vana
 * Copyright (c) 2012 Martin Decky
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
#include <stdbool.h>
#include <str.h>
#include <abi/sysinfo.h>

/** Framebuffer info exported flags */
extern bool fb_exported;

/** Subtree type
 *
 */
typedef enum {
	SYSINFO_SUBTREE_NONE = 0,     /**< No subtree (leaf item) */
	SYSINFO_SUBTREE_TABLE = 1,    /**< Fixed subtree */
	SYSINFO_SUBTREE_FUNCTION = 2  /**< Generated subtree */
} sysinfo_subtree_type_t;

struct sysinfo_item;

/** Generated numeric value function */
typedef sysarg_t (*sysinfo_fn_val_t)(struct sysinfo_item *, void *);

/** Sysinfo generated numberic value data
 *
 */
typedef struct {
	sysinfo_fn_val_t fn;  /**< Generated value function */
	void *data;           /**< Private data */
} sysinfo_gen_val_data_t;

/** Generated binary data function */
typedef void *(*sysinfo_fn_data_t)(struct sysinfo_item *, size_t *, bool,
    void *);

/** Sysinfo generated binary data data
 *
 */
typedef struct {
	sysinfo_fn_data_t fn;  /**< Generated binary data function */
	void *data;            /**< Private data */
} sysinfo_gen_data_data_t;

/** Sysinfo item binary data
 *
 */
typedef struct {
	void *data;   /**< Data */
	size_t size;  /**< Size (bytes) */
} sysinfo_data_t;

/** Sysinfo item value (union)
 *
 */
typedef union {
	sysarg_t val;                      /**< Constant numberic value */
	sysinfo_data_t data;               /**< Constant binary data */
	sysinfo_gen_val_data_t gen_val;    /**< Generated numeric value function */
	sysinfo_gen_data_data_t gen_data;  /**< Generated binary data function */
} sysinfo_item_val_t;

/** Sysinfo return holder
 *
 * This structure is generated from the constant
 * items or by the generating functions. Note that
 * the validity of the data is limited by the scope
 * of single sysinfo invocation guarded by sysinfo_lock.
 *
 */
typedef struct {
	sysinfo_item_val_type_t tag;  /**< Return value type */
	union {
		sysarg_t val;             /**< Numberic value */
		sysinfo_data_t data;      /**< Binary data */
	};
} sysinfo_return_t;

/** Generated subtree function */
typedef sysinfo_return_t (*sysinfo_fn_subtree_t)(const char *, bool, void *);

/** Sysinfo generated subtree data
 *
 */
typedef struct {
	sysinfo_fn_subtree_t fn;  /**< Generated subtree function */
	void *data;               /**< Private data */
} sysinfo_gen_subtree_data_t;

/** Sysinfo subtree (union)
 *
 */
typedef union {
	struct sysinfo_item *table;            /**< Fixed subtree (list of subitems) */
	sysinfo_gen_subtree_data_t generator;  /**< Generated subtree */
} sysinfo_subtree_t;

/** Sysinfo item
 *
 */
typedef struct sysinfo_item {
	char *name;                           /**< Item name */
	
	sysinfo_item_val_type_t val_type;     /**< Item value type */
	sysinfo_item_val_t val;               /**< Item value */
	
	sysinfo_subtree_type_t subtree_type;  /**< Subtree type */
	sysinfo_subtree_t subtree;            /**< Subtree */
	
	struct sysinfo_item *next;            /**< Sibling item */
} sysinfo_item_t;

extern void sysinfo_set_item_val(const char *, sysinfo_item_t **, sysarg_t);
extern void sysinfo_set_item_data(const char *, sysinfo_item_t **, void *,
    size_t);
extern void sysinfo_set_item_gen_val(const char *, sysinfo_item_t **,
    sysinfo_fn_val_t, void *);
extern void sysinfo_set_item_gen_data(const char *, sysinfo_item_t **,
    sysinfo_fn_data_t, void *);
extern void sysinfo_set_item_undefined(const char *, sysinfo_item_t **);

extern void sysinfo_set_subtree_fn(const char *, sysinfo_item_t **,
    sysinfo_fn_subtree_t, void *);

extern void sysinfo_init(void);
extern void sysinfo_dump(sysinfo_item_t *);

extern sys_errno_t sys_sysinfo_get_keys_size(void *, size_t, void *);
extern sys_errno_t sys_sysinfo_get_keys(void *, size_t, void *, size_t, size_t *);
extern sysarg_t sys_sysinfo_get_val_type(void *, size_t);
extern sys_errno_t sys_sysinfo_get_value(void *, size_t, void *);
extern sys_errno_t sys_sysinfo_get_data_size(void *, size_t, void *);
extern sys_errno_t sys_sysinfo_get_data(void *, size_t, void *, size_t, size_t *);

#endif

/** @}
 */
