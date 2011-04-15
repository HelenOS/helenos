/*
 * Copyright (c) 2009 Lukas Mejdrech
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
 *  @{
 */

/** @file
 *  Generic type field.
 */

#ifndef LIBC_GENERIC_FIELD_H_
#define LIBC_GENERIC_FIELD_H_

#include <errno.h>
#include <malloc.h>
#include <mem.h>
#include <unistd.h>

/** Internal magic value for a&nbsp;field consistency check. */
#define GENERIC_FIELD_MAGIC_VALUE		0x55667788

/** Generic destructor function pointer. */
#define DTOR_T(identifier) \
	void (*identifier)(const void *)

/** Generic type field declaration.
 *
 * @param[in] name	Name of the field.
 * @param[in] type	Inner object type.
 */
#define GENERIC_FIELD_DECLARE(name, type) \
	typedef	struct name name##_t; \
	\
	struct	name { \
		int size; \
		int next; \
		type **items; \
		int magic; \
	}; \
	\
	int name##_add(name##_t *, type *); \
	int name##_count(name##_t *); \
	void name##_destroy(name##_t *, DTOR_T()); \
	void name##_exclude_index(name##_t *, int, DTOR_T()); \
	type **name##_get_field(name##_t *); \
	type *name##_get_index(name##_t *, int); \
	int name##_initialize(name##_t *); \
	int name##_is_valid(name##_t *);

/** Generic type field implementation.
 *
 * Should follow declaration with the same parameters.
 *
 * @param[in] name	Name of the field.
 * @param[in] type	Inner object type.
 */
#define GENERIC_FIELD_IMPLEMENT(name, type) \
	int name##_add(name##_t *field, type *value) \
	{ \
		if (name##_is_valid(field)) { \
			if (field->next == (field->size - 1)) { \
				type **tmp; \
				tmp = (type **) realloc(field->items, \
				    sizeof(type *) * 2 * field->size); \
				if (!tmp) \
					return ENOMEM; \
				field->size *= 2; \
				field->items = tmp; \
			} \
			field->items[field->next] = value; \
			field->next++; \
			field->items[field->next] = NULL; \
			return field->next - 1; \
		} \
		return EINVAL; \
	} \
	\
	int name##_count(name##_t *field) \
	{ \
		return name##_is_valid(field) ? field->next : -1; \
	} \
	\
	void name##_destroy(name##_t *field, DTOR_T(dtor)) \
	{ \
		if (name##_is_valid(field)) { \
			int index; \
			field->magic = 0; \
			if (dtor) { \
				for (index = 0; index < field->next; index++) { \
					if (field->items[index]) \
						dtor(field->items[index]); \
				} \
			} \
			free(field->items); \
		} \
	} \
	 \
	void name##_exclude_index(name##_t *field, int index, DTOR_T(dtor)) \
	{ \
		if (name##_is_valid(field) && (index >= 0) && \
		    (index < field->next) && (field->items[index])) { \
			if (dtor) \
				dtor(field->items[index]); \
			field->items[index] = NULL; \
		} \
	} \
	 \
	type *name##_get_index(name##_t *field, int index) \
	{ \
		if (name##_is_valid(field) && (index >= 0) && \
		    (index < field->next) && (field->items[index])) \
			return field->items[index]; \
		return NULL; \
	} \
	\
	type **name##_get_field(name##_t *field) \
	{ \
		return name##_is_valid(field) ? field->items : NULL; \
	} \
	\
	int name##_initialize(name##_t *field) \
	{ \
		if (!field) \
			return EINVAL; \
		field->size = 2; \
		field->next = 0; \
		field->items = (type **) malloc(sizeof(type *) * field->size); \
		if (!field->items) \
			return ENOMEM; \
		field->items[field->next] = NULL; \
		field->magic = GENERIC_FIELD_MAGIC_VALUE; \
		return EOK; \
	} \
	\
	int name##_is_valid(name##_t *field) \
	{ \
		return field && (field->magic == GENERIC_FIELD_MAGIC_VALUE); \
	}

#endif

/** @}
 */
