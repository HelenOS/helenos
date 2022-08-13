/*
 * SPDX-FileCopyrightText: 2006 Jakub Vana
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
 * @{
 */
/** @file
 */

#ifndef KERN_BFB_H_
#define KERN_BFB_H_

#include <stdbool.h>
#include <typedefs.h>

extern uintptr_t bfb_addr;
extern uint32_t bfb_width;
extern uint32_t bfb_height;
extern uint16_t bfb_bpp;
extern uint32_t bfb_scanline;

extern uint8_t bfb_red_pos;
extern uint8_t bfb_red_size;

extern uint8_t bfb_green_pos;
extern uint8_t bfb_green_size;

extern uint8_t bfb_blue_pos;
extern uint8_t bfb_blue_size;

extern bool bfb_init(void);

#endif

/** @}
 */
