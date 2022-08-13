/*
 * SPDX-FileCopyrightText: 2018 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup uspace_drv_gicv2
 * @{
 */
/** @file
 */

#ifndef GICV2_H_
#define GICV2_H_

#include <ddf/driver.h>
#include <loc.h>
#include <stdint.h>

typedef struct {
	uintptr_t distr_base;
	uintptr_t cpui_base;
} gicv2_res_t;

/** GICv2 interrupt controller. */
typedef struct {
	ddf_dev_t *dev;
	void *distr;
	void *cpui;
	unsigned max_irq;
} gicv2_t;

extern errno_t gicv2_add(gicv2_t *, gicv2_res_t *);
extern errno_t gicv2_remove(gicv2_t *);
extern errno_t gicv2_gone(gicv2_t *);

#endif

/** @}
 */
