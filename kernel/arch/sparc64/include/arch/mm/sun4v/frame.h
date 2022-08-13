/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_mm
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_sun4v_FRAME_H_
#define KERN_sparc64_sun4v_FRAME_H_

#define MMU_FRAME_WIDTH  13  /* 8K */
#define MMU_FRAME_SIZE   (1 << MMU_FRAME_WIDTH)

#define FRAME_WIDTH  13
#define FRAME_SIZE   (1 << FRAME_WIDTH)

#define FRAME_LOWPRIO  0

#endif

/** @}
 */
