/*
 * SPDX-FileCopyrightText: 2013 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdrv
 * @{
 */
/** @file
 */

#ifndef LIBDRV_OPS_PIO_WINDOW_H_
#define LIBDRV_OPS_PIO_WINDOW_H_

#include <device/pio_window.h>
#include "../ddf/driver.h"

typedef struct {
	pio_window_t *(*get_pio_window)(ddf_fun_t *);
} pio_window_ops_t;

#endif

/**
 * @}
 */
