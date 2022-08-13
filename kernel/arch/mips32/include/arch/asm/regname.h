/*
 * SPDX-FileCopyrightText: 2005 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32
 * @{
 */
/** @file
 */

#ifndef KERN_mips32_REGNAME_H_
#define KERN_mips32_REGNAME_H_

#define zero    0
#define at      1
#define v0      2
#define v1      3
#define a0      4
#define a1      5
#define a2      6
#define a3      7
#define t0      8
#define t1      9
#define t2      10
#define t3      11
#define t4      12
#define t5      13
#define t6      14
#define t7      15
#define s0      16
#define s1      17
#define s2      18
#define s3      19
#define s4      20
#define s5      21
#define s6      22
#define s7      23
#define t8      24
#define t9      25
#define k0      26
#define k1      27
#define gp      28
#define sp      29
#define s8      30
#define ra      31

#define rindex    0
#define rrandom   1
#define entrylo0  2
#define entrylo1  3
#define context   4
#define pagemask  5
#define wired     6
#define badvaddr  8
#define count     9
#define entryhi   10
#define compare   11
#define status    12
#define cause     13
#define epc       14
#define rconfig   16
#define lladdr    17
#define watchlo   18
#define watchhi   19
#define xcontext  20
#define rdebug    23
#define depc      24
#define eepc      30

#endif

/** @}
 */
