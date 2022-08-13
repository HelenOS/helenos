/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
 * @{
 */
/** @file
 */

#ifndef KERN_EGA_H_
#define KERN_EGA_H_

#include <typedefs.h>
#include <console/chardev.h>

#define EGA_COLS       80
#define EGA_ROWS       25
#define EGA_SCREEN     (EGA_COLS * EGA_ROWS)
#define EGA_VRAM_SIZE  (2 * EGA_SCREEN)

/* EGA device registers. */
#define EGA_INDEX_REG  0
#define EGA_DATA_REG   1

extern outdev_t *ega_init(ioport8_t *, uintptr_t);

#endif

/** @}
 */
