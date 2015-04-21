/*
 * Copyright (c) 2015 Jan Kolarik
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

/** 
 * @addtogroup libieee80211
 * @{
 */

/** @file ieee80211_impl.h
 * 
 * IEEE 802.11 default device functions definition.
 */

#ifndef LIB_IEEE80211_IMPL_H
#define	LIB_IEEE80211_IMPL_H

#include "ieee80211_private.h"

extern int ieee80211_start_impl(ieee80211_dev_t *ieee80211_dev);
extern int ieee80211_tx_handler_impl(ieee80211_dev_t *ieee80211_dev, 
	void *buffer, size_t buffer_size);
extern int ieee80211_set_freq_impl(ieee80211_dev_t *ieee80211_dev, 
	uint16_t freq);
extern int ieee80211_bssid_change_impl(ieee80211_dev_t *ieee80211_dev,
	bool connected);
extern int ieee80211_key_config_impl(ieee80211_dev_t *ieee80211_dev,
	ieee80211_key_config_t *key_conf, bool insert);
extern int ieee80211_scan_impl(ieee80211_dev_t *ieee80211_dev);
extern int ieee80211_prf(uint8_t *key, uint8_t *data, uint8_t *hash, 
	size_t output_size);
extern int ieee80211_rc4_key_unwrap(uint8_t *key, uint8_t *data, 
	size_t data_size, uint8_t *output);
extern int ieee80211_aes_key_unwrap(uint8_t *kek, uint8_t *data, 
	size_t data_size, uint8_t *output);
extern int ieee80211_michael_mic(uint8_t *key, uint8_t *data, size_t data_size, 
	uint8_t *mic);
extern uint16_t uint16le_from_seq(void *seq);
extern uint32_t uint32le_from_seq(void *seq);
extern uint16_t uint16be_from_seq(void *seq);
extern uint32_t uint32be_from_seq(void *seq);
extern int rnd_sequence(uint8_t *sequence, size_t length);
extern uint8_t *min_sequence(uint8_t *seq1, uint8_t *seq2, size_t size);
extern uint8_t *max_sequence(uint8_t *seq1, uint8_t *seq2, size_t size);

#endif	/* LIB_IEEE80211_IMPL_H */

/** @}
 */