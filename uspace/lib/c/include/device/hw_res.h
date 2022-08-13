/*
 * SPDX-FileCopyrightText: 2010 Lenka Trochtova
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_DEVICE_HW_RES_H_
#define _LIBC_DEVICE_HW_RES_H_

#include <ipc/dev_iface.h>
#include <async.h>
#include <stdbool.h>

#define DMA_MODE_ON_DEMAND  0
#define DMA_MODE_WRITE      (1 << 2)
#define DMA_MODE_READ       (1 << 3)
#define DMA_MODE_AUTO       (1 << 4)
#define DMA_MODE_DOWN       (1 << 5)
#define DMA_MODE_SINGLE     (1 << 6)
#define DMA_MODE_BLOCK      (1 << 7)

/** HW resource provider interface */
typedef enum {
	HW_RES_GET_RESOURCE_LIST = 0,
	HW_RES_ENABLE_INTERRUPT,
	HW_RES_DISABLE_INTERRUPT,
	HW_RES_CLEAR_INTERRUPT,
	HW_RES_DMA_CHANNEL_SETUP,
	HW_RES_DMA_CHANNEL_REMAIN,
} hw_res_method_t;

/** HW resource types */
typedef enum {
	INTERRUPT,
	IO_RANGE,
	MEM_RANGE,
	DMA_CHANNEL_8,
	DMA_CHANNEL_16,
} hw_res_type_t;

typedef enum {
	LITTLE_ENDIAN = 0,
	BIG_ENDIAN
} endianness_t;

/** HW resource (e.g. interrupt, memory register, i/o register etc.) */
typedef struct {
	hw_res_type_t type;
	union {
		struct {
			uint64_t address;
			size_t size;
			bool relative;
			endianness_t endianness;
		} mem_range;

		struct {
			uint64_t address;
			size_t size;
			bool relative;
			endianness_t endianness;
		} io_range;

		struct {
			int irq;
		} interrupt;

		union {
			unsigned int dma8;
			unsigned int dma16;
		} dma_channel;
	} res;
} hw_resource_t;

typedef struct {
	size_t count;
	hw_resource_t *resources;
} hw_resource_list_t;

static inline void hw_res_clean_resource_list(hw_resource_list_t *hw_res)
{
	if (hw_res->resources != NULL) {
		free(hw_res->resources);
		hw_res->resources = NULL;
	}

	hw_res->count = 0;
}

extern errno_t hw_res_get_resource_list(async_sess_t *, hw_resource_list_t *);
extern errno_t hw_res_enable_interrupt(async_sess_t *, int);
extern errno_t hw_res_disable_interrupt(async_sess_t *, int);
extern errno_t hw_res_clear_interrupt(async_sess_t *, int);

extern errno_t hw_res_dma_channel_setup(async_sess_t *, unsigned int, uint32_t,
    uint32_t, uint8_t);
extern errno_t hw_res_dma_channel_remain(async_sess_t *, unsigned, size_t *);

#endif

/** @}
 */
