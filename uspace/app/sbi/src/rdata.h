/*
 * Copyright (c) 2010 Jiri Svoboda
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
rdata_ref_t *rdata_ref_new(void);
rdata_deleg_t *rdata_deleg_new(void);
rdata_array_t *rdata_array_new(int rank);
rdata_object_t *rdata_object_new(void);
rdata_bool_t *rdata_bool_new(void);
rdata_char_t *rdata_char_new(void);
rdata_int_t *rdata_int_new(void);
rdata_string_t *rdata_string_new(void);
rdata_resource_t *rdata_resource_new(void);

void rdata_array_alloc_element(rdata_array_t *array);
void rdata_var_copy(rdata_var_t *src, rdata_var_t **dest);

void rdata_var_read(rdata_var_t *var, rdata_item_t **ritem);
void rdata_var_write(rdata_var_t *var, rdata_value_t *value);

void rdata_item_print(rdata_item_t *item);
void rdata_value_print(rdata_value_t *value);

#endif
