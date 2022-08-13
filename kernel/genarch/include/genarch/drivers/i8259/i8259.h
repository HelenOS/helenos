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

#ifndef KERN_I8259_H_
#define KERN_I8259_H_

#include <typedefs.h>
#include <arch/interrupt.h>
#include <genarch/pic/pic_ops.h>
#include <stdbool.h>

typedef struct {
	ioport8_t port1;
	ioport8_t port2;
} __attribute__((packed)) i8259_t;

extern pic_ops_t i8259_pic_ops;

extern void i8259_init(i8259_t *, i8259_t *, unsigned int);
extern void i8259_enable_irqs(uint16_t);
extern void i8259_disable_irqs(uint16_t);
extern void i8259_eoi(unsigned int);
extern bool i8259_is_spurious(unsigned int);
extern void i8259_handle_spurious(unsigned int);

#endif

/** @}
 */
