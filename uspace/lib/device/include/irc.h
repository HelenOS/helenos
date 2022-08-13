/*
 * SPDX-FileCopyrightText: 2014 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdevice
 * @{
 */
/** @file
 */

#ifndef LIBDEVICE_IRC_H
#define LIBDEVICE_IRC_H

extern errno_t irc_enable_interrupt(int);
extern errno_t irc_disable_interrupt(int);
extern errno_t irc_clear_interrupt(int);

#endif

/** @}
 */
