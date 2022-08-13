/*
 * SPDX-FileCopyrightText: 2010 Lenka Trochtova
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdrv
 * @{
 */
/** @file
 */

#ifndef LIBDRV_OPS_HW_RES_H_
#define LIBDRV_OPS_HW_RES_H_

#include <device/hw_res.h>
#include <stddef.h>
#include <stdint.h>
#include "../ddf/driver.h"

typedef struct {
	hw_resource_list_t *(*get_resource_list)(ddf_fun_t *);
	errno_t (*enable_interrupt)(ddf_fun_t *, int);
	errno_t (*disable_interrupt)(ddf_fun_t *, int);
	errno_t (*clear_interrupt)(ddf_fun_t *, int);
	errno_t (*dma_channel_setup)(ddf_fun_t *, unsigned, uint32_t, uint32_t, uint8_t);
	errno_t (*dma_channel_remain)(ddf_fun_t *, unsigned, size_t *);
} hw_res_ops_t;

#endif

/**
 * @}
 */
