/*
 * SPDX-FileCopyrightText: 2015 Jan Kolarik
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file hw.h
 *
 * Definitions of AR9271 hardware related functions.
 *
 */

#ifndef ATHEROS_HW_H
#define ATHEROS_HW_H

#include "ar9271.h"

#define HW_WAIT_LOOPS    1000
#define HW_WAIT_TIME_US  10

extern errno_t hw_init(ar9271_t *);
extern errno_t hw_freq_switch(ar9271_t *, uint16_t);
extern errno_t hw_rx_init(ar9271_t *);
extern errno_t hw_reset(ar9271_t *);
extern errno_t hw_set_bssid(ar9271_t *);
extern errno_t hw_set_rx_filter(ar9271_t *, bool);

#endif  /* ATHEROS_HW_H */
