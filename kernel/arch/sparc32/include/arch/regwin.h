/*
 * Copyright (c) 2005 Jakub Jermar
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

/** @addtogroup sparc32interrupt
 * @{
 */
/**
 * @file
 * @brief This file contains register window trap handlers.
 */

#ifndef KERN_sparc32_REGWIN_H_
#define KERN_sparc32_REGWIN_H_

#include <arch/stack.h>
#include <arch/arch.h>
#include <align.h>

/* Window Save Area offsets. */
#define L0_OFFSET  0
#define L1_OFFSET  4
#define L2_OFFSET  8
#define L3_OFFSET  12
#define L4_OFFSET  16
#define L5_OFFSET  20
#define L6_OFFSET  24
#define L7_OFFSET  28
#define I0_OFFSET  32
#define I1_OFFSET  36
#define I2_OFFSET  40
#define I3_OFFSET  44
#define I4_OFFSET  48
#define I5_OFFSET  52
#define I6_OFFSET  56
#define I7_OFFSET  60

/* User space Window Buffer constants. */
#define UWB_SIZE       ((NWINDOWS - 1) * STACK_WINDOW_SAVE_AREA_SIZE)
#define UWB_ALIGNMENT  1024
#define UWB_ASIZE      ALIGN_UP(UWB_SIZE, UWB_ALIGNMENT)

#endif

/** @}
 */
