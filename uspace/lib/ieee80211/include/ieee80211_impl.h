/*
 * SPDX-FileCopyrightText: 2015 Jan Kolarik
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
#define LIB_IEEE80211_IMPL_H

#include "ieee80211_private.h"

extern errno_t ieee80211_start_impl(ieee80211_dev_t *);
extern errno_t ieee80211_tx_handler_impl(ieee80211_dev_t *, void *, size_t);
extern errno_t ieee80211_set_freq_impl(ieee80211_dev_t *, uint16_t);
extern errno_t ieee80211_bssid_change_impl(ieee80211_dev_t *, bool);
extern errno_t ieee80211_key_config_impl(ieee80211_dev_t *,
    ieee80211_key_config_t *, bool);
extern errno_t ieee80211_scan_impl(ieee80211_dev_t *);
extern errno_t ieee80211_prf(uint8_t *, uint8_t *, uint8_t *, size_t);
extern errno_t ieee80211_rc4_key_unwrap(uint8_t *, uint8_t *, size_t, uint8_t *);
extern errno_t ieee80211_aes_key_unwrap(uint8_t *, uint8_t *, size_t, uint8_t *);
extern errno_t ieee80211_michael_mic(uint8_t *, uint8_t *, size_t, uint8_t *);
extern uint16_t uint16le_from_seq(void *);
extern uint32_t uint32le_from_seq(void *);
extern uint16_t uint16be_from_seq(void *);
extern uint32_t uint32be_from_seq(void *);
extern errno_t rnd_sequence(uint8_t *, size_t);
extern uint8_t *min_sequence(uint8_t *, uint8_t *, size_t);
extern uint8_t *max_sequence(uint8_t *, uint8_t *, size_t);

#endif  /* LIB_IEEE80211_IMPL_H */

/** @}
 */
