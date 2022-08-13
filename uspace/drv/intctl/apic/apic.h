/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup uspace_drv_apic
 * @{
 */
/** @file
 */

#ifndef APIC_H_
#define APIC_H_

#include <ddf/driver.h>
#include <ddi.h>
#include <loc.h>
#include <stdint.h>

typedef struct {
	uintptr_t base;
} apic_res_t;

/** APIC */
typedef struct {
	ioport32_t *regs;
	uintptr_t phys_base;
	ddf_dev_t *dev;
} apic_t;

extern errno_t apic_add(apic_t *, apic_res_t *);
extern errno_t apic_remove(apic_t *);
extern errno_t apic_gone(apic_t *);

#endif

/** @}
 */
