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

#ifndef RDATA_H_
#define RDATA_H_

#include "mytypes.h"

rdata_item_t *rdata_item_new(item_class_t ic);
rdata_addr_var_t *rdata_addr_var_new(void);
rdata_aprop_named_t *rdata_aprop_named_new(void);
rdata_aprop_indexed_t *rdata_aprop_indexed_new(void);
rdata_addr_prop_t *rdata_addr_prop_new(aprop_class_t apc);
rdata_address_t *rdata_address_new(address_class_t ac);
rdata_value_t *rdata_value_new(void);

rdata_var_t *rdata_var_new(var_class_t vc);
void rdata_var_delete(rdata_var_t *var);
rdata_ref_t *rdata_ref_new(void);
rdata_deleg_t *rdata_deleg_new(void);
rdata_enum_t *rdata_enum_new(void);
rdata_array_t *rdata_array_new(int rank);
rdata_object_t *rdata_object_new(void);
rdata_bool_t *rdata_bool_new(void);
rdata_char_t *rdata_char_new(void);
rdata_int_t *rdata_int_new(void);
rdata_string_t *rdata_string_new(void);
rdata_resource_t *rdata_resource_new(void);
rdata_symbol_t *rdata_symbol_new(void);

void rdata_address_delete(rdata_address_t *address);
void rdata_value_delete(rdata_value_t *value);
void rdata_var_delete(rdata_var_t *var);
void rdata_item_delete(rdata_item_t *item);
void rdata_addr_var_delete(rdata_addr_var_t *addr_var);
void rdata_addr_prop_delete(rdata_addr_prop_t *addr_prop);
void rdata_aprop_named_delete(rdata_aprop_named_t *aprop_named);
void rdata_aprop_indexed_delete(rdata_aprop_indexed_t *aprop_indexed);

void rdata_bool_delete(rdata_bool_t *bool_v);
void rdata_char_delete(rdata_char_t *char_v);
void rdata_int_delete(rdata_int_t *int_v);
void rdata_string_delete(rdata_string_t *string_v);
void rdata_ref_delete(rdata_ref_t *ref_v);
void rdata_deleg_delete(rdata_deleg_t *deleg_v);
void rdata_enum_delete(rdata_enum_t *enum_v);
void rdata_array_delete(rdata_array_t *array_v);
void rdata_object_delete(rdata_object_t *object_v);
void rdata_resource_delete(rdata_resource_t *resource_v);
void rdata_symbol_delete(rdata_symbol_t *symbol_v);

void rdata_array_alloc_element(rdata_array_t *array);
void rdata_value_copy(rdata_value_t *val, rdata_value_t **rval);
void rdata_var_copy(rdata_var_t *src, rdata_var_t **dest);

void rdata_address_destroy(rdata_address_t *address);
void rdata_addr_var_destroy(rdata_addr_var_t *addr_var);
void rdata_addr_prop_destroy(rdata_addr_prop_t *addr_prop);
void rdata_aprop_named_destroy(rdata_aprop_named_t *aprop_named);
void rdata_aprop_indexed_destroy(rdata_aprop_indexed_t *aprop_indexed);
void rdata_value_destroy(rdata_value_t *value);
void rdata_var_destroy(rdata_var_t *var);
void rdata_item_destroy(rdata_item_t *item);

void rdata_var_read(rdata_var_t *var, rdata_item_t **ritem);
void rdata_var_write(rdata_var_t *var, rdata_value_t *value);

void rdata_item_print(rdata_item_t *item);
void rdata_value_print(rdata_value_t *value);

#endif
