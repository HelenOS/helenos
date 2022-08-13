/*
 * SPDX-FileCopyrightText: 2015 Jan Kolarik
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @addtogroup libieee80211
 * @{
 */

/** @file ieee80211_iface_impl.h
 *
 * IEEE 802.11 default interface functions definition.
 */

#ifndef LIB_IEEE80211_IFACE_IMPL_H
#define LIB_IEEE80211_IFACE_IMPL_H

#include <ddf/driver.h>
#include "ieee80211.h"

extern errno_t ieee80211_get_scan_results_impl(ddf_fun_t *,
    ieee80211_scan_results_t *, bool);
extern errno_t ieee80211_connect_impl(ddf_fun_t *, char *, char *);
extern errno_t ieee80211_disconnect_impl(ddf_fun_t *);

#endif  /* LIB_IEEE80211_IFACE_IMPL_H */

/** @}
 */
