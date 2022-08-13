/*
 * SPDX-FileCopyrightText: 2019 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
 * @{
 */
/** @file
 */

#ifndef KERN_PIC_OPS_H_
#define KERN_PIC_OPS_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	const char *(*get_name)(void);
	void (*disable_irqs)(uint16_t);
	void (*enable_irqs)(uint16_t);
	void (*eoi)(unsigned int);
	bool (*is_spurious)(unsigned int);
	void (*handle_spurious)(unsigned int);
} pic_ops_t;

extern pic_ops_t *pic_ops;

#endif

/** @}
 */
