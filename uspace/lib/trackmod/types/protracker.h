/*
 * SPDX-FileCopyrightText: 2014 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
