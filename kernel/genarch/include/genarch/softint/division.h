/*
 * SPDX-FileCopyrightText: 2006 Josef Cejka
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
 * @{
 */
/** @file
 */

#ifndef KERN_DIVISION_H_
#define KERN_DIVISION_H_

#include <cc.h>

#ifdef CONFIG_LTO
#define DO_NOT_DISCARD ATTRIBUTE_USED
#else
#define DO_NOT_DISCARD
#endif

extern int __divsi3(int, int);
extern long long __divdi3(long long, long long);

extern unsigned int __udivsi3(unsigned int, unsigned int);
extern unsigned long long __udivdi3(unsigned long long, unsigned long long) DO_NOT_DISCARD;

extern int __modsi3(int, int);
extern long long __moddi3(long long, long long);

extern unsigned int __umodsi3(unsigned int, unsigned int);
extern unsigned long long __umoddi3(unsigned long long, unsigned long long) DO_NOT_DISCARD;

extern int __divmodsi3(int, int, int *);
extern unsigned int __udivmodsi3(unsigned int, unsigned int, unsigned int *);

extern long long __divmoddi3(long long, long long, long long *);
extern unsigned long long __udivmoddi3(unsigned long long, unsigned long long,
    unsigned long long *);
extern unsigned long long __udivmoddi4(unsigned long long, unsigned long long,
    unsigned long long *);

#endif

/** @}
 */
