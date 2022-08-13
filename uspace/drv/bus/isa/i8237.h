/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup isa
 * @{
 */

/** @file
 * @brief DMA memory management
 */

#ifndef DRV_BUS_ISA_I8237_H
#define DRV_BUS_ISA_I8237_H

extern errno_t dma_channel_setup(unsigned, uint32_t, uint32_t, uint8_t);
extern errno_t dma_channel_remain(unsigned, size_t *);

#endif

/**
 * @}
 */
