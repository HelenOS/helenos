/*
 * SPDX-FileCopyrightText: 2011 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_PRIVATE_IO_H_
#define _LIBC_PRIVATE_IO_H_

#include <loader/pcb.h>

extern void __stdio_init(void);
extern void __stdio_done(void);

extern void __inbox_init(struct pcb_inbox_entry *entries, int count);

#endif

/** @}
 */
