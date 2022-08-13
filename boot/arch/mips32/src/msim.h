/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BOOT_mips32_MSIM_H_
#define BOOT_mips32_MSIM_H_

extern void init(void);
extern void halt(void);

extern void *translate(void *addr);

#endif
