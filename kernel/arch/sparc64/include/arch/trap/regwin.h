/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_interrupt
 * @{
 */
/**
 * @file
 * @brief This file contains register window trap handlers.
 */

#ifndef KERN_sparc64_REGWIN_H_
#define KERN_sparc64_REGWIN_H_

#include <arch/stack.h>
#include <arch/arch.h>
#include <align.h>

#define TT_CLEAN_WINDOW			0x24
#define TT_SPILL_0_NORMAL		0x80	/* kernel spills */
#define TT_SPILL_1_NORMAL		0x84	/* userspace spills */
#define TT_SPILL_2_NORMAL		0x88	/* spills to userspace window buffer */
#define TT_SPILL_0_OTHER		0xa0	/* spills to userspace window buffer */
#define TT_FILL_0_NORMAL		0xc0	/* kernel fills */
#define TT_FILL_1_NORMAL		0xc4	/* userspace fills */

#define REGWIN_HANDLER_SIZE		128

#define CLEAN_WINDOW_HANDLER_SIZE	REGWIN_HANDLER_SIZE
#define SPILL_HANDLER_SIZE		REGWIN_HANDLER_SIZE
#define FILL_HANDLER_SIZE		REGWIN_HANDLER_SIZE

/* Window Save Area offsets. */
#define L0_OFFSET	0
#define L1_OFFSET	8
#define L2_OFFSET	16
#define L3_OFFSET	24
#define L4_OFFSET	32
#define L5_OFFSET	40
#define L6_OFFSET	48
#define L7_OFFSET	56
#define I0_OFFSET	64
#define I1_OFFSET	72
#define I2_OFFSET	80
#define I3_OFFSET	88
#define I4_OFFSET	96
#define I5_OFFSET	104
#define I6_OFFSET	112
#define I7_OFFSET	120

/* Uspace Window Buffer constants. */
#define UWB_SIZE	((NWINDOWS - 1) * STACK_WINDOW_SAVE_AREA_SIZE)
#define UWB_ALIGNMENT	1024

#ifdef __ASSEMBLER__
#include <arch/trap/regwin.S>
#endif /* __ASSEMBLER__ */

#if defined(SUN4U)
#include <arch/trap/sun4u/regwin.h>
#elif defined(SUN4V)
#include <arch/trap/sun4v/regwin.h>
#endif

#endif

/** @}
 */
