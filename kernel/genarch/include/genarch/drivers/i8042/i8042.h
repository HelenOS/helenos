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

#ifndef KERN_I8042_H_
#define KERN_I8042_H_

#include <ddi/irq.h>
#include <console/chardev.h>
#include <typedefs.h>

typedef struct {
	ioport8_t data;
	uint8_t pad[3];
	ioport8_t status;
} __attribute__((packed)) i8042_t;

typedef struct {
	irq_t irq;
	i8042_t *i8042;
	indev_t *kbrdin;
} i8042_instance_t;

extern i8042_instance_t *i8042_init(i8042_t *, inr_t);
extern void i8042_wire(i8042_instance_t *, indev_t *);
extern void i8042_cpu_reset(i8042_t *);

#endif

/** @}
 */
