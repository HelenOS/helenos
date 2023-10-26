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

#ifndef DWARFS_CONSTANTS_H_
#define DWARFS_CONSTANTS_H_

enum {
	DW_SECT_INFO        = 1,  // .debug_info.dwo
	DW_SECT_ABBREV      = 3,  // .debug_abbrev.dwo
	DW_SECT_LINE        = 4,  // .debug_line.dwo
	DW_SECT_LOCLISTS    = 5,  // .debug_loclists.dwo
	DW_SECT_STR_OFFSETS = 6,  // .debug_str_offsets.dwo
	DW_SECT_MACRO       = 7,  // .debug_macro.dwo
	DW_SECT_RNGLISTS    = 8,  // .debug_rnglists.dwo
};

// ^(DW_[^ ]*) [‡† ]*([^ ]*).*

#define CVAL(name, value) name = value,

enum {
#include "constants/dw_ut.inc"

	DW_UT_lo_user = 0x80,
	DW_UT_hi_user = 0xff,
};

enum {
#include "constants/dw_tag.inc"

	DW_TAG_lo_user = 0x4080,
	DW_TAG_hi_user = 0xffff,
};

enum {
#include "constants/dw_at.inc"

	DW_AT_lo_user = 0x2000,
	DW_AT_hi_user = 0x3fff,
};

enum {
#include "constants/dw_form.inc"
};

enum {
#include "constants/dw_op.inc"

	DW_OP_lo_user = 0xe0,
	DW_OP_hi_user = 0xff,
};

enum {
#include "constants/dw_lle.inc"
};

enum {
#include "constants/dw_ate.inc"

	DW_ATE_lo_user = 0x80,
	DW_ATE_hi_user = 0xff,
};

enum {
#include "constants/dw_ds.inc"
};

enum {
#include "constants/dw_end.inc"

	DW_END_lo_user = 0x40,
	DW_END_hi_user = 0xff,
};

enum {
#include "constants/dw_access.inc"
};

enum {
#include "constants/dw_vis.inc"
};

enum {
#include "constants/dw_virtuality.inc"
};

enum {
#include "constants/dw_lang.inc"

	DW_LANG_lo_user = 0x8000,
	DW_LANG_hi_user = 0xffff,
};

enum {
	DW_ADDR_none = 0,
};

enum {
#include "constants/dw_id.inc"
};

enum {
#include "constants/dw_cc.inc"

	DW_CC_lo_user = 0x40,
	DW_CC_hi_user = 0xff,
};

enum {
#include "constants/dw_lns.inc"
};

enum {
#include "constants/dw_lne.inc"

	DW_LNE_lo_user = 0x80,
	DW_LNE_hi_user = 0xff,
};

enum {
#include "constants/dw_lnct.inc"

	DW_LNCT_lo_user = 0x2000,
	DW_LNCT_hi_user = 0x3fff,
};

#undef CVAL

typedef unsigned dw_ut_t;
typedef unsigned dw_tag_t;
typedef unsigned dw_at_t;
typedef unsigned dw_form_t;
typedef unsigned dw_op_t;
typedef unsigned dw_lle_t;
typedef unsigned dw_ate_t;
typedef unsigned dw_ds_t;
typedef unsigned dw_end_t;
typedef unsigned dw_access_t;
typedef unsigned dw_vis_t;
typedef unsigned dw_virtuality_t;
typedef unsigned dw_lang_t;
typedef unsigned dw_id_t;
typedef unsigned dw_cc_t;
typedef unsigned dw_lns_t;
typedef unsigned dw_lne_t;
typedef unsigned dw_lnct_t;

#endif /* DWARFS_CONSTANTS_H_ */
