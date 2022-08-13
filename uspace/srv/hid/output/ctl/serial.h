/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 * SPDX-FileCopyrightText: 2008 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup output
 * @{
 */

#ifndef OUTPUT_CTL_SERIAL_H_
#define OUTPUT_CTL_SERIAL_H_

#include "../proto/vt100.h"

extern errno_t serial_init(vt100_putuchar_t, vt100_control_puts_t, vt100_flush_t);

#endif

/** @}
 */
