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

#include <arch/types.h>

typedef union sysinfo_item_val {
	unative_t val;
	void *fn; 
} sysinfo_item_val_t;

typedef struct sysinfo_item {
	char *name;
	union {
		unative_t val;
		void *fn; 
	} val;

	union {
		struct sysinfo_item *table;
		void *fn;
	} subinfo;

	struct sysinfo_item *next;
	int val_type;
	int subinfo_type;
} sysinfo_item_t;

#define SYSINFO_VAL_VAL 0
#define SYSINFO_VAL_FUNCTION 1
#define SYSINFO_VAL_UNDEFINED '?'

#define SYSINFO_SUBINFO_NONE 0
#define SYSINFO_SUBINFO_TABLE 1
#define SYSINFO_SUBINFO_FUNCTION 2


typedef unative_t (*sysinfo_val_fn_t)(sysinfo_item_t *root);
typedef unative_t (*sysinfo_subinfo_fn_t)(const char *subname);

typedef struct sysinfo_rettype {
	unative_t val;
	unative_t valid;
} sysinfo_rettype_t;

void sysinfo_set_item_val(const char *name,sysinfo_item_t **root,unative_t val);
void sysinfo_dump(sysinfo_item_t **root,int depth);
void sysinfo_set_item_function(const char *name,sysinfo_item_t **root,sysinfo_val_fn_t fn);
void sysinfo_set_item_undefined(const char *name,sysinfo_item_t **root);

sysinfo_rettype_t sysinfo_get_val(const char *name,sysinfo_item_t **root);

unative_t sys_sysinfo_valid(unative_t ptr,unative_t len);
unative_t sys_sysinfo_value(unative_t ptr,unative_t len);

/** @}
 */
