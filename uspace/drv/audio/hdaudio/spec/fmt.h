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

/** @addtogroup hdaudio
 * @{
 */
/** @file High Definition Audio stream format
 */

#ifndef SPEC_FMT_H
#define SPEC_FMT_H

typedef enum {
	/** Stream Type */
	fmt_type = 15,
	/** Sample Base Rate */
	fmt_base = 14,
	/** Sample Base Rate Multiple (H) */
	fmt_mult_h = 13,
	/** Sample Base Rate Multiple (L) */
	fmt_mult_l = 11,
	/** Sample Base Rate Divisor (H) */
	fmt_div_h = 10,
	/** Sample Base Rate Divisor (L) */
	fmt_div_l = 8,
	/** Bits per Sample (H) */
	fmt_bits_h = 6,
	/** Bits per Sample (L) */
	fmt_bits_l = 4,
	/** Number of Channels (H) */
	fmt_chan_h = 3,
	/** Number of Channels (L) */
	fmt_chan_l = 0
} hda_stream_fmt_bits_t;

/** Stream Type */
typedef enum {
	/** PCM */
	fmt_type_pcm = 0,
	/** Non-PCM */
	fmt_type_nonpcm = 1
} hda_fmt_type_t;

/** Sample Base Rate */
typedef enum {
	/** 48 kHz */
	fmt_base_48khz = 0,
	/** 44.1 kHz */
	fmt_base_44khz = 1
} hda_fmt_base_t;

/** Sample Base Rate Multiple */
typedef enum {
	/** 48 kHz/44.1 kHz or less */
	fmt_mult_x1 = 0,
	/** x2 (96 kHz, 88.2 kHz, 32 kHz) */
	fmt_mult_x2 = 1,
	/** x3 (144 kHz) */
	fmt_mult_x3 = 2,
	/** x4 (192 kHz, 176.4 kHz) */
	fmt_mult_x4 = 3
} hda_fmt_mult_t;

/** Sample Base Rate Divisor */
typedef enum {
	/** Divide by 1 (48 kHz, 44.1 kHz) */
	fmt_div_1 = 0,
	/** Divide by 2 (24 kHz, 22.05 kHz) */
	fmt_div_2 = 1,
	/** Divide by 3 (16 kHz, 32 kHz) */
	fmt_div_3 = 2,
	/** Divide by 4 (11.025 kHz) */
	fmt_div_4 = 3,
	/** Divide by 5 (9.6 kHz) */
	fmt_div_5 = 4,
	/** Divide by 6 (8 kHz) */
	fmt_div_6 = 5,
	/** Divide by 7 */
	fmt_div_7 = 6,
	/** Divide by 8 (6 kHz) */
	fmt_div_8 = 7
} hda_fmt_div_t;

/** Bits per Sample */
typedef enum {
	/** 8 bits */
	fmt_bits_8 = 0,
	/** 16 bits */
	fmt_bits_16 = 1,
	/** 20 bits */
	fmt_bits_20 = 2,
	/** 24 bits */
	fmt_bits_24 = 3,
	/** 32 bits */
	fmt_bits_32 = 4
} hda_fmt_bits_t;

/** Number of Channels */
typedef enum {
	fmt_chan_1 = 0,
	fmt_chan_2 = 1,
	fmt_chan_3 = 2,
	fmt_chan_4 = 3,
	fmt_chan_5 = 4,
	fmt_chan_6 = 5,
	fmt_chan_7 = 6,
	fmt_chan_8 = 7,
	fmt_chan_9 = 8,
	fmt_chan_10 = 9,
	fmt_chan_11 = 10,
	fmt_chan_12 = 11,
	fmt_chan_13 = 12,
	fmt_chan_14 = 13,
	fmt_chan_15 = 14,
	fmt_chan_16 = 15
} hda_fmt_chan_t;

#endif

/** @}
 */
