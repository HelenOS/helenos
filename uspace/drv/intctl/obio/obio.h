/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup obio
 * @{
 */
/** @file
 */

#ifndef OBIO_H_
#define OBIO_H_

#include <ddf/driver.h>
#include <ddi.h>
#include <loc.h>
#include <stdint.h>

typedef struct {
	uintptr_t base;
} obio_res_t;

/** OBIO */
typedef struct {
	ioport64_t *regs;
	uintptr_t phys_base;
	ddf_dev_t *dev;
} obio_t;

extern errno_t obio_add(obio_t *, obio_res_t *);
extern errno_t obio_remove(obio_t *);
extern errno_t obio_gone(obio_t *);

#endif

/** @}
 */
