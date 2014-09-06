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
	hda_param_get = 0xf00,
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
	hda_spdif_ctl_set2 = 0x70e,
	/** S/PDIF Converter Control / Set 3 */
	hda_spdif_ctl_set3 = 0x73e,
	/** S/PDIF Converter Control / Set 4 */
	hda_spdif_ctl_set4 = 0x73f,
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
	/** Pin Control / Get */
	hda_pin_ctl_get = 0xf07,
	/** Pin Control / Set */
	hda_pin_ctl_set = 0x707,
	/** Unsolicited Response Control / Get */
	hda_unsol_resp_get = 0xf08,
	/** Unsolicied Response Control / Set */
	hda_unsol_resp_set = 0x708,
	/** Pin Sense / Get */
	hda_pin_sense_get = 0xf09,
	/** Pin Sense / Execute */
	hda_pin_sense_exec = 0x709,
	/** EAPD/BTL Enable / Get */
	hda_eapd_btl_enable_get = 0xf0c,
	/** EAPD/BTL Enable / Set */
	hda_eapd_btl_enable_set = 0x70c,
	/** GPI Data / Get */
	hda_gpi_data_get = 0xf10,
	/** GPI Data / Set */
	hda_gpi_data_set = 0x710,
	/** GPI Wake Enable / Get */
	hda_gpi_wakeen_get = 0xf11,
	/** GPI Wake Enable / Set */
	hda_gpi_wakeen_set = 0x711,
	/** GPI Unsolicited Enable / Get */
	hda_gpi_unsol_get = 0xf12,
	/** GPI Unsolicited Enable / Set */
	hda_gpi_unsol_set = 0x712,
	/** GPI Sticky / Get */
	hda_gpi_sticky_get = 0xf13,
	/** GPI Sticky / Set */
	hda_gpi_sticky_set = 0x713,
	/** GPO Data / Get */
	hda_gpo_data_get = 0xf14,
	/** GPO Data / Set */
	hda_gpo_data_set = 0x714,
	/** GPIO Data / Get */
	hda_gpio_data_get = 0xf15,
	/** GPIO Data / Set */
	hda_gpio_data_set = 0x715,
	/** GPIO Enable / Get */
	hda_gpio_enable_get = 0xf16,
	/** GPIO Enable / Set */
	hda_gpio_enable_set = 0x716,
	/** GPIO Direction / Get */
	hda_gpio_dir_get = 0xf17,
	/** GPIO Direction / Set */
	hda_gpio_dir_set = 0x717,
	/** GPIO Wake Enable / Get */
	hda_gpio_wakeen_get = 0xf18,
	/** GPIO Wake Enable / Set */
	hda_gpio_wakeen_set = 0x718,
	/** GPIO Unsolicited Enable / Get */
	hda_gpio_unsol_get = 0xf19,
	/** GPIO Unsolicited Enable / Set */
	hda_gpio_unsol_set = 0x719,
	/** GPIO Sticky Mask / Get */
	hda_gpio_sticky_get = 0xf1a,
	/** GPIO Sticky Mask / Set */
	hda_hpio_sticky_set = 0x71a,
	/** Beep Generation / Get */
	hda_beep_gen_get = 0xf0a,
	/** Beep Generation / Set */
	hda_beep_gen_set = 0x70a,
	/** Volume Knob / Get */
	hda_vol_knob_get = 0xf0f,
	/** Volume Knob / Set */
	hda_vol_knob_set = 0x70f,
	/** Implementation Identification / Get */
	hda_impl_ident_get = 0xf20,
	/** Implementation Identification / Set 1 */
	hda_impl_ident_set1 = 0x720,
	/** Implementation Identification / Set 2 */
	hda_impl_ident_set2 = 0x721,
	/** Implementation Identification / Set 3 */
	hda_impl_ident_set3 = 0x722,
	/** Implementation Identification / Set 4 */
	hda_impl_ident_set4 = 0x723,
	/** Configuration Default / Get */
	hda_cfg_def_get = 0xf1c,
	/** Configuration Default / Set 1 */
	hda_cfg_def_set1 = 0x71c,
	/** Configuration Default / Set 2 */
	hda_cfg_def_set2 = 0x71d,
	/** Configuration Default / Set 3 */
	hda_cfg_def_set3 = 0x71e,
	/** Configuration Default / Set 4 */
	hda_cfg_def_set4 = 0x71f,
	/** Stripe Control / Get */
	hda_stripe_ctl_get = 0xf24,
	/** Stripe Control / Set */
	hda_stripe_ctl_set = 0x724,
	/** Function Reset / Execute */
	hda_fun_rst_exec = 0x7ff,
	/** ELD Data / Get */
	hda_eld_data_get = 0xf2f,
	/** Converter Channel Count / Get */
	hda_cvt_chan_cnt_get = 0xf2d,
	/** Converter Channel Count / Set */
	hda_cvt_chan_cnt_set = 0x72d,
	/** DIP-Size / Get */
	hda_dip_size_get = 0xf2e,
	/** DIP-Index / Get */
	hda_dip_index_get = 0xf30,
	/** DIP-Index / Set */
	hda_dip_index_set = 0x730,
	/** DIP-Data / Get */
	hda_dip_data_get = 0xf31,
	/** DIP-Data / Set */
	hda_dip_data_set = 0x731,
	/** DIP-XmitCtrl / Get */
	hda_dip_xmitctrl_get = 0xf32,
	/** DIP-XmitCtrl / Set */
	hda_dip_xmitctrl_set = 0x732,
	/** Protection Control / Get */
	hda_prot_ctl_get = 0xf33,
	/** Protection Control / Set */
	hda_prot_ctl_set = 0x733,
	/** ASP Channel Mapping / Get */
	hda_asp_chanmap_get = 0xf34,
	/** ASP Channel Mapping / Set */
	hda_asp_chanmap_set = 0x734
} hda_verb_t;

typedef enum {
	/** Vendor ID */
	hda_vendor_id = 0x00,
	/** Revision ID */
	hda_revision_id = 0x02,
	/** Subordinate Node Count */
	hda_sub_nc = 0x04,
	/** Function Group Type */
	hda_fgrp_type = 0x05,
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
	hda_clist_len = 0x0e,
	/** Supported Power States */
	hda_supp_pwr_states = 0x0f,
	/** Processing Capabilities */
	hda_proc_caps = 0x10,
	/** GP I/O Count */
	hda_gpio_cnt = 0x11,
	/** Volume Knob Capabilities */
	hda_volk_nob_caps = 0x13
} hda_param_id_t;

/** Subordinate Node Count Response bits */
typedef enum {
	/** Starting Node Number (H) */
	subnc_startnode_h = 23,
	/** Starting Node Number (L) */
	subnc_startnode_l = 16,
	/** Total Node Count (H) */
	subnc_nodecount_h = 7,
	/** Total Node Count (L) */
	subnc_nodecount_l = 0
} hda_sub_nc_bits_t;

/** Function Group Type Response bits */
typedef enum {
	/** UnSol Capable */
	fgrpt_unsol = 8,
	/** Group Type (H) */
	fgrpt_type_h = 7,
	/** Group Type (L) */
	fgrpt_type_l = 0
} hda_fgrp_type_bits_t;

/** Function Group Type */
typedef enum {
	/** Audio Function Group */
	fgrp_afg = 0x01,
	/** Vendor Defined Modem Function Group */
	fgrp_vdmfg = 0x02
} hda_fgrp_type_t;

/** Connection List Length Response bits */
typedef enum {
	/** Long Form */
	cll_longform = 7,
	/** Connection List Length (H) */
	cll_len_h = 6,
	/** Connection List Length (L) */
	cll_len_l = 0
} hda_clist_len_bits_t;

/** Audio Widget Capabilities Bits */
typedef enum {
	/** Type (H) */
	awc_type_h = 23,
	/** Type (L) */
	awc_type_l = 20,
	/** Chan Count Ext (H) */
	awc_chan_count_ext_h = 15,
	/** Chan Count Ext (L) */
	awc_chan_count_ext_l = 13,
	/** CP Caps */
	awc_cp_caps = 12,
	/** L-R Swap */
	awc_lr_swap = 11,
	/** Power Control */
	awc_power_cntrl = 10,
	/** Digital */
	awc_digital = 9,
	/** Conn List */
	awc_conn_list = 8,
	/** Unsol Capable */
	awc_unsol_capable = 7,
	/** Proc Widget */
	awc_proc_widget = 6,
	/** Stripe */
	awc_stripe = 5,
	/** Format Override */
	awc_fmt_override = 4,
	/** Amp Param Override */
	awc_amp_param_override = 3,
	/** Out Amp Present */
	awc_out_amp_present = 2,
	/** In Amp Present */
	awc_in_amp_present = 1,
	/** Chan Count LSB (Stereo) */
	awc_chan_count_lsb = 0
} hda_awidget_caps_bits_t;

/** Pin Capabilities */
typedef enum {
	/** High Bit Rate */
	pwc_hbr = 27,
	/** Display Port */
	pwc_dp = 24,
	/** EAPD Capable */
	pwc_eapd = 16,
	/** VRef Control (H) */
	pwc_vrefctl_h = 15,
	/** VRef Control (L) */
	pwc_vrefctl_l = 8,
	/** HDMI */
	pwc_hdmi = 7,
	/** Balanced I/O Pins */
	pwc_bal_io = 6,
	/** Input Capable */
	pwc_input = 5,
	/** Output Capable */
	pwc_output = 4,
	/** Headphone Drive Capable */
	pwc_hpd = 3,
	/** Presence Detect Capable */
	pwc_presence = 2,
	/** Trigger Required */
	pwc_trigger_reqd = 1,
	/** Impedance Sense Capable */
	pwc_imp_sense = 0
} hda_pin_caps_bits_t;

/** Audio Widget Type */
typedef enum {
	/** Audio Output */
	awt_audio_output = 0x0,
	/** Audio Input */
	awt_audio_input = 0x1,
	/** Audio Mixer */
	awt_audio_mixer = 0x2,
	/** Audio Selector */
	awt_audio_selector = 0x3,
	/** Pin Complex */
	awt_pin_complex = 0x4,
	/** Power Widget */
	awt_power_widget = 0x5,
	/** Volume Knob Widget */
	awt_volume_knob = 0x6,
	/** Beep Generator Widget */
	awt_beep_generator = 0x7,
	/** Vendor-defined audio widget */
	awt_vendor_defined = 0xf
} hda_awidget_type_t;

/** Converter Control bits */
typedef enum {
	/** Stream (H) */
	cctl_stream_h = 7,
	/** Stream (L) */
	cctl_stream_l = 4,
	/** Channel (H) */
	cctl_channel_h = 3,
	/** Channel (L) */
	cctl_channel_l = 0
} hda_converter_ctl_bits_t;

/** Pin Widget Control bits */
typedef enum {
	/** Headphone Drive Enable */
	pctl_hpd_enable = 7,
	/** Out Enable */
	pctl_out_enable = 6,
	/** In Enable */
	pctl_in_enable = 5,
	/** Voltage Reference Enable (H) */
	pctl_vref_enable_h = 2,
	/** Voltage Reference Enable (L) */
	pctl_vref_enable_l = 0,
	/** Encoded Packet Type (H) */
	pctl_ept_h = 1,
	/** Encoded Packet Type (L) */
	pctl_ept_l = 0
} hda_pin_ctl_bits_t;

#endif

/** @}
 */
