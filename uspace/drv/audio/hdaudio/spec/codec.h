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
/** @file High Definition Audio codec interface
 */

#ifndef SPEC_CODEC_H
#define SPEC_CODEC_H

typedef enum {
	/** Get Parameter */
	hda_get_param = 0xf00,
	/** Connection Select Control / Get */
	hda_conn_sel_get = 0xf01,
	/** Connection Select Control / Set */
	hda_conn_sel_set = 0x701,
	/** Get Connection List Entry */
	hda_clist_entry_get = 0xf02,
	/** Processing State / Get */
	hda_proc_state_get = 0xf03,
	/** Processing State / Set */
	hda_proc_state_set = 0x703,
	/** Coefficient Index / Get */
	hda_coef_index_get = 0xd,
	/** Coefficient Index / Set */
	hda_coef_index_set = 0x5,
	/** Processing Coefficient / Get */
	hda_proc_coef_get = 0xc,
	/** Processing Coefficient / Set */
	hda_proc_coef_set = 0x4,
	/** Amplifier Gain/Mute / Get */
	hda_amp_gain_mute_get = 0xb,
	/** Amplifier Gain/Mute / Set */
	hda_amp_gain_mute_set = 0x3,
	/** Converter Format / Get */
	hda_converter_fmt_get = 0xa,
	/** Converter Format / Set */
	hda_converter_fmt_set = 0x2,
	/** S/PDIF Converter Control / Get */
	hda_spdif_ctl_get = 0xf0d,
	/** S/PDIF Converter Control / Set 1 */
	hda_spdif_ctl_set1 = 0x70d,
	/** S/PDIF Converter Control / Set 2 */
	hda_spdif_ctl_set1 = 0x70e,
	/** S/PDIF Converter Control / Set 3 */
	hda_spdif_ctl_set1 = 0x73e,
	/** S/PDIF Converter Control / Set 4 */
	hda_spdif_ctl_set1 = 0x73f,
	/** Power State / Get */
	hda_power_state_get = 0xf05,
	/** Power State / Set */
	hda_power_state_set = 0x705,
	/** Converter Control / Get */
	hda_converter_ctl_get = 0xf06,
	/** Converter Control / Set */
	hda_converter_ctl_set = 0x706,
	/** SDI Select / Get */
	hda_sdi_select_get = 0xf04,
	/** SDI Select / Set */
	hda_sdi_select_set = 0x704,
	/** Enable VRef / Get */
	hda_enable_vref_get = 0xf07,
	/** Enable VRef / Set */
	hda_enable_vref_set = 0x707,
	/** Connection Select Control / Get */
	hda_conn_sel_get = 0xf08,
	/** Connection Select Control / Set */
	hda_conn_sel_set = 0x708
} hda_verb_t;

typedef enum {
	/** Vendor ID */
	hda_vendor_id = 0x00,
	/** Revision ID */
	hda_revision_id = 0x02,
	/** Subordinate Node Count */
	hda_sub_nc = 0x04,
	/** Function Group Type */
	hba_fgrp_type = 0x05,
	/** Audio Function Group Capabilities */
	hda_afg_caps = 0x08,
	/** Audio Widget Capabilities */
	hda_aw_caps = 0x09,
	/** Supported PCM Size, Rates */
	hda_supp_rates = 0x0a,
	/** Supported Stream Formats */
	hda_supp_formats = 0x0b,
	/** Pin Capabilities */
	hda_pin_caps = 0x0c,
	/** Input Amplifier Capabilities */
	hda_in_amp_caps = 0x0d,
	/** Output Amplifier Capabilities */
	hda_out_amp_caps = 0x12,
	/** Connection List Length */
	hda_clist_len = 0xe,
	/** Supported Power States */
	hda_supp_pwr_states = 0x0f,
	/** Processing Capabilities */
	hda_proc_caps = 0x10,
	/** GP I/O Count */
	hda_gpio_cnt = 0x11,
	/** Volume Knob Capabilities */
	hda_volk_nob_caps = 0x13
} hda_param_id_t;

#endif

/** @}
 */
