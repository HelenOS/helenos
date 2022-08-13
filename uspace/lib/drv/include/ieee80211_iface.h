/*
 * SPDX-FileCopyrightText: 2015 Jan Kolarik
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdrv
 * @{
 */
/** @file ieee80211_iface.h
 */

#ifndef LIBDRV_IEEE80211_IFACE_H_
#define LIBDRV_IEEE80211_IFACE_H_

#include <ieee80211/ieee80211.h>
#include <async.h>

extern errno_t ieee80211_get_scan_results(async_sess_t *,
    ieee80211_scan_results_t *, bool);
extern errno_t ieee80211_connect(async_sess_t *, char *, char *);
extern errno_t ieee80211_disconnect(async_sess_t *);

#endif

/** @}
 */
