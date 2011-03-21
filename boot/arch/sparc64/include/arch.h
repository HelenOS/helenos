/*
 * Copyright (c) 2006 Martin Decky
 * Copyright (c) 2006 Jakub Jermar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef BOOT_sparc64_ARCH_H_
#define BOOT_sparc64_ARCH_H_

#define PAGE_WIDTH  14
#define PAGE_SIZE   (1 << PAGE_WIDTH)

#define LOADER_ADDRESS  0x004000
#define KERNEL_ADDRESS  0x400000

#define STACK_SIZE                   8192
#define STACK_ALIGNMENT              16
#define STACK_BIAS                   2047
#define STACK_WINDOW_SAVE_AREA_SIZE  (16 * 8)
#define STACK_ARG_SAVE_AREA_SIZE     (6 * 8)

#define NWINDOWS  8

#define PSTATE_IE_BIT    2
#define PSTATE_PRIV_BIT  4
#define PSTATE_AM_BIT    8

#define ASI_ICBUS_CONFIG        0x4a
#define ICBUS_CONFIG_MID_SHIFT  17

/** Constants to distinguish particular UltraSPARC architecture */
#define ARCH_SUN4U  10
#define ARCH_SUN4V  20

/** Constants to distinguish particular UltraSPARC subarchitecture */
#define SUBARCH_UNKNOWN  0
#define SUBARCH_US       1
#define SUBARCH_US3      3

#define BSP_PROCESSOR  1
#define AP_PROCESSOR   0

#endif
