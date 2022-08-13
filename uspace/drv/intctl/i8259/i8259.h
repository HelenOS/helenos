/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup i8259
 * @{
 */
/** @file
 */

#ifndef I8259_H_
#define I8259_H_

#include <ddf/driver.h>
#include <ddi.h>
#include <stdint.h>

typedef struct {
	uintptr_t base0;
	uintptr_t base1;
} i8259_res_t;

/** IntegratorCP Interrupt Controller */
typedef struct {
	ioport8_t *regs0;
	ioport8_t *regs1;
	ddf_dev_t *dev;
} i8259_t;

extern errno_t i8259_add(i8259_t *, i8259_res_t *);
extern errno_t i8259_remove(i8259_t *);
extern errno_t i8259_gone(i8259_t *);

#endif

/** @}
 */
