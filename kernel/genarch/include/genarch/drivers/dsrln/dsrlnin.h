/*
 * SPDX-FileCopyrightText: 2009 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief Dummy serial line input.
 */

#ifndef KERN_DSRLNIN_H_
#define KERN_DSRLNIN_H_

#include <ddi/irq.h>
#include <console/chardev.h>
#include <typedefs.h>

typedef struct {
	ioport8_t data;
} __attribute__((packed)) dsrlnin_t;

typedef struct {
	irq_t irq;
	dsrlnin_t *dsrlnin;
	indev_t *srlnin;
} dsrlnin_instance_t;

extern dsrlnin_instance_t *dsrlnin_init(dsrlnin_t *, inr_t);
extern void dsrlnin_wire(dsrlnin_instance_t *, indev_t *);

#endif

/** @}
 */
