/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup icp-ic
 * @{
 */
/** @file
 */

#ifndef ICP_IC_H_
#define ICP_IC_H_

#include <ddf/driver.h>
#include <loc.h>
#include <stdint.h>

#include "icp-ic_hw.h"

typedef struct {
	uintptr_t base;
} icpic_res_t;

/** IntegratorCP Interrupt Controller */
typedef struct {
	icpic_regs_t *regs;
	uintptr_t phys_base;
	ddf_dev_t *dev;
} icpic_t;

extern errno_t icpic_add(icpic_t *, icpic_res_t *);
extern errno_t icpic_remove(icpic_t *);
extern errno_t icpic_gone(icpic_t *);

#endif

/** @}
 */
