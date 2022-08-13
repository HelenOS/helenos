/*
 * SPDX-FileCopyrightText: Maurizio Lombardi
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief Texas Instruments OMAP UART driver.
 */

#ifndef _KERN_OMAP_UART_H_
#define _KERN_OMAP_UART_H_

#include "uart_regs.h"

typedef struct {
	omap_uart_regs_t *regs;
	indev_t *indev;
	outdev_t outdev;
	irq_t irq;
} omap_uart_t;

extern bool omap_uart_init(omap_uart_t *uart, inr_t interrupt,
    uintptr_t addr, size_t size);

extern void omap_uart_input_wire(omap_uart_t *uart, indev_t *indev);

#endif

/**
 * @}
 */
