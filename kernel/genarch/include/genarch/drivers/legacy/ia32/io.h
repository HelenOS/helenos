/*
 * SPDX-FileCopyrightText: 2009 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32
 * @{
 */
/** @file
 * @brief This file contains definitions used by architectures with the
 *        ia32 legacy I/O space (i.e. ia32, amd64 and ia64).
 */

#ifndef KERN_LEGACY_IA32_IO_H
#define KERN_LEGACY_IA32_IO_H

#include <typedefs.h>

#define I8042_BASE    ((ioport8_t *) 0x60)
#define EGA_BASE      ((ioport8_t *) 0x3d4)
#define NS16550_BASE  ((ioport8_t *) 0x3f8)

#define I8259_PIC0_BASE ((ioport8_t *) 0x20U)
#define I8259_PIC1_BASE ((ioport8_t *) 0xA0U)

#define EGA_VIDEORAM  0xb8000

#endif

/** @}
 */
