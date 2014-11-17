/*
 * Copyright (c) 2014 Jiri Svoboda
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

/** @addtogroup trackmod
 * @{
 */
/**
 * @file Protracker module (.mod) types.
 */

#ifndef TYPES_PROTRACKER_H
#define TYPES_PROTRACKER_H

#include <stdint.h>

enum {
	/** Module name size */
	protracker_mod_name_size = 20,
	/** Sample name size */
	protracker_smp_name_size = 22,
	/** Order list max length */
	protracker_olist_len = 128,
	/** Number of rows in a pattern */
	protracker_pattern_rows = 64,
	/** Default TPR */
	protracker_def_tpr = 6,
	/** Default BPM */
	protracker_def_bpm = 125
};

/** Protracker initial header. */
typedef struct {
	/** Module name */
	uint8_t name[protracker_mod_name_size];
} protracker_hdr_t;

/** Protracker sample header. */
typedef struct {
	/** Sample name, padded with zeros */
	uint8_t name[protracker_smp_name_size];
	/** Sample length in words */
	uint16_t length;
	/** Finetune value */
	uint8_t finetune;
	/** Default volume */
	uint8_t def_vol;
	/** Loop start in words */
	uint16_t loop_start;
	/** Loop length in words */
	uint16_t loop_len;
} protracker_smp_t;

/** Protracker order list. */
typedef struct {
	/** Number of used entries */
	uint8_t order_list_len;
	uint8_t mark;
	/** Order list */
	uint8_t order_list[protracker_olist_len];
} protracker_order_list_t;

/** Protracker 15-sample variant headers. */
typedef struct {
	/** Initial header */
	protracker_hdr_t hdr;
	/** Sample headers. */
	protracker_smp_t sample[15];
	/** Order list. */
	protracker_order_list_t order_list;
} protracker_15smp_t;

/** Protracker 31-sample variant headers. */
typedef struct {
	/** Initial header */
	protracker_hdr_t hdr;
	/** Sample headers. */
	protracker_smp_t sample[31];
	/** Order list. */
	protracker_order_list_t order_list;
	/** Sample tag. */
	uint8_t sample_tag[4];
} protracker_31smp_t;

#endif

/** @}
 */
