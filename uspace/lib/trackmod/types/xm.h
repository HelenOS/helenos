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
 * @file Extended Module (.xm) types.
 */

#ifndef TYPES_XM_H
#define TYPES_XM_H

#include <stdint.h>

enum {
	/** ID text (signature) */
	xm_id_text_size = 17,
	/** Module name size */
	xm_mod_name_size = 20,
	/** Tracker name size */
	xm_tracker_name_size = 20,
	/** Pattern order table size */
	xm_pat_ord_table_size = 256,
	/** Instrument name size */
	xm_instr_name_size = 22,
	/** Sample number for all notes table size */
	xm_smp_note_size = 96,
	/** Max number of volume envelope points */
	xm_vol_env_points = 48,
	/** Max number of panning envelope points */
	xm_pan_env_points = 48,
	/** Sample name size */
	xm_smp_name_size = 22,
	/** Keyoff note number */
	xm_keyoff_note = 97
};

/** XM file header */
typedef struct {
	uint8_t id_text[xm_id_text_size];
	/** Module name */
	uint8_t name[xm_mod_name_size];
	/** Text EOF mark */
	uint8_t text_break;
	/** Tracker name */
	uint8_t tracker_name[xm_tracker_name_size];
	/** File format version */
	uint16_t version;
	/** Header size */
	uint32_t hdr_size;
	/** Song length (in pattern order table) */
	uint16_t song_len;
	/** Restart position */
	uint16_t restart_pos;
	/** Number of channels */
	uint16_t channels;
	/** Number of patterns */
	uint16_t patterns;
	/** Number of intstruments */
	uint16_t instruments;
	/** Flags */
	uint16_t flags;
	/** Default tempo */
	uint16_t def_tempo;
	/** Default BPM */
	uint16_t def_bpm;
	uint8_t pat_ord_table[xm_pat_ord_table_size];
} __attribute__((packed)) xm_hdr_t;

typedef enum {
	/** 1 = Linear frequency table, 0 = Amiga freq. table */
	xmf_lftable = 0
} xm_flags_bits_t;

/** XM pattern header */
typedef struct {
	/** Pattern header size */
	uint32_t hdr_size;
	/** Packing type */
	uint8_t pack_type;
	/** Number of rows */
	uint16_t rows;
	/** Packed pattern data size */
	uint16_t data_size;
} __attribute__((packed)) xm_pattern_t;

/** XM instrument header. */
typedef struct {
	/** Instrument size */
	uint32_t size;
	/** Instrument name */
	uint8_t name[xm_instr_name_size];
	/** Instrument type */
	uint8_t instr_type;
	/** Number of samples in instrument */
	uint16_t samples;
} __attribute__((packed)) xm_instr_t;

/** XM additional instrument header if number of samples > 0 */
typedef struct {
	/** Sample header size */
	uint32_t smp_hdr_size;
	/** Sample number for all notes */
	uint8_t smp_note[xm_smp_note_size];
	/** Points for volume envelope */
	uint8_t vol_point[xm_vol_env_points];
	/** Points for panning envelope */
	uint8_t pan_point[xm_pan_env_points];
	/** Number of volume points */
	uint8_t vol_points;
	/** Number of panning points */
	uint8_t pan_points;
	/** Volume sustating point */
	uint8_t vol_sustain;
	/** Volume loop start point */
	uint8_t vol_loop_start;
	/** Volume loop end point */
	uint8_t vol_loop_end;
	/** Panning sustating point */
	uint8_t pan_sustain;
	/** Panning loop start point */
	uint8_t pan_loop_start;
	/** Panning loop end point */
	uint8_t pan_loop_end;
	/** Volume type */
	uint8_t vol_type;
	/** Panning type */
	uint8_t pan_type;
	/** Vibrato type */
	uint8_t vibrato_type;
	/** Vibrato sweep */
	uint8_t vibrato_sweep;
	/** Vibrato depth */
	uint8_t vibrato_depth;
	/** Vibrato rate */
	uint8_t vibrato_rate;
	/** Volume fadeout */
	uint16_t vol_fadeout;
	/** Reserved */
	uint16_t res241;
} __attribute__((packed)) xm_instr_ext_t;

/** XM sample header */
typedef struct {
	/** Sample length */
	uint32_t length;
	/** Loop start */
	uint32_t loop_start;
	/** Loop length */
	uint32_t loop_len;
	/** Volume */
	uint8_t volume;
	/** Finetune */
	int8_t finetune;
	/** Sample type */
	uint8_t smp_type;
	/** Panning */
	uint8_t panning;
	/** Relative note number */
	int8_t rel_note;
	/** Reserved */
	uint8_t res17;
	/** Sample name */
	uint8_t name[xm_smp_name_size];
} __attribute__((packed)) xm_smp_t;

/** XM sample type bits */
typedef enum {
	/** 16-bit sample data */
	xmst_16_bit = 4,
	/** Loop type (H) */
	xmst_loop_type_h = 1,
	/** Loop type (L) */
	xmst_loop_type_l = 0
} xm_smp_type_bits_t;

/** Sample loop type */
typedef enum {
	/** No loop */
	xmsl_no_loop = 0,
	/** Forward loop */
	xmsl_forward_loop = 1,
	/** Ping-pong loop */
	xmsl_pingpong_loop = 2
} xm_smp_loop_type;

#endif

/** @}
 */
