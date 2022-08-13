/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_CONSOLE_H_
#define KERN_sparc64_CONSOLE_H_

extern void kkbdpoll(void *arg);
extern void standalone_sparc64_console_init(void);

#endif

/** @}
 */
