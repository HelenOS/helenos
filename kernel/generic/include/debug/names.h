/*
 * Copyright (c) 2023 Jiří Zárevúcky
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

#ifndef DEBUG_DWARF_NAMES_H_
#define DEBUG_DWARF_NAMES_H_

#include <debug/types.h>

extern const char *dw_access_name(dw_access_t);
extern const char *dw_at_name(dw_at_t);
extern const char *dw_ate_name(dw_ate_t);
extern const char *dw_cc_name(dw_cc_t);
extern const char *dw_cfa_name(dw_cfa_t);
extern const char *dw_defaulted_name(dw_defaulted_t);
extern const char *dw_ds_name(dw_ds_t);
extern const char *dw_dsc_name(dw_dsc_t);
extern const char *dw_end_name(dw_end_t);
extern const char *dw_form_name(dw_form_t);
extern const char *dw_id_name(dw_id_t);
extern const char *dw_idx_name(dw_idx_t);
extern const char *dw_inl_name(dw_inl_t);
extern const char *dw_lang_name(dw_lang_t);
extern const char *dw_lle_name(dw_lle_t);
extern const char *dw_lnct_name(dw_lnct_t);
extern const char *dw_lne_name(dw_lne_t);
extern const char *dw_lns_name(dw_lns_t);
extern const char *dw_macro_name(dw_macro_t);
extern const char *dw_op_name(dw_op_t);
extern const char *dw_ord_name(dw_ord_t);
extern const char *dw_rle_name(dw_rle_t);
extern const char *dw_tag_name(dw_tag_t);
extern const char *dw_ut_name(dw_ut_t);
extern const char *dw_virtuality_name(dw_virtuality_t);
extern const char *dw_vis_name(dw_vis_t);

#endif /* DEBUG_DWARF_NAMES_H_ */
