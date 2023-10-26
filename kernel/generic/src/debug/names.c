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

#include <stddef.h>
#include <stdint.h>
#include <debug/constants.h>
#include <debug/names.h>

#define CVAL(name, value) [value] = #name,

static const char *const dw_ut_names[] = {
#include <debug/constants/dw_ut.inc>
};

static const char *const dw_tag_names[] = {
#include <debug/constants/dw_tag.inc>
};

static const char *const dw_at_names[] = {
#include <debug/constants/dw_at.inc>
};

static const char *const dw_form_names[] = {
#include <debug/constants/dw_form.inc>
};

static const char *const dw_op_names[] = {
#include <debug/constants/dw_op.inc>
};

static const char *const dw_lle_names[] = {
#include <debug/constants/dw_lle.inc>
};

static const char *const dw_ate_names[] = {
#include <debug/constants/dw_ate.inc>
};

static const char *const dw_ds_names[] = {
#include <debug/constants/dw_ds.inc>
};

static const char *const dw_end_names[] = {
#include <debug/constants/dw_end.inc>
};

static const char *const dw_access_names[] = {
#include <debug/constants/dw_access.inc>
};

static const char *const dw_vis_names[] = {
#include <debug/constants/dw_vis.inc>
};

static const char *const dw_virtuality_names[] = {
#include <debug/constants/dw_virtuality.inc>
};

static const char *const dw_lang_names[] = {
#include <debug/constants/dw_lang.inc>
};

static const char *const dw_id_names[] = {
#include <debug/constants/dw_id.inc>
};

static const char *const dw_cc_names[] = {
#include <debug/constants/dw_cc.inc>
};

static const char *const dw_lns_names[] = {
#include <debug/constants/dw_lns.inc>
};

static const char *const dw_lne_names[] = {
#include <debug/constants/dw_lne.inc>
};

static const char *const dw_lnct_names[] = {
#include <debug/constants/dw_lnct.inc>
};

#undef CVAL

#define D_(infix) \
		const char *dw_##infix##_name(dw_##infix##_t val) { \
			if (val >= sizeof(dw_##infix##_names) / sizeof(const char *)) \
				return NULL; \
			return dw_##infix##_names[val]; \
		}

D_(ut);
D_(tag);
D_(at);
D_(form);
D_(op);
D_(lle);
D_(ate);
D_(ds);
D_(end);
D_(access);
D_(vis);
D_(virtuality);
D_(lang);
D_(id);
D_(cc);
D_(lns);
D_(lne);
D_(lnct);

#undef D_
