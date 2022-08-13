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
 * @brief Texas Instruments AM335x UART driver.
 */

#ifndef _KERN_AM335X_UART_H_
#define _KERN_AM335X_UART_H_

#include <genarch/drivers/omap/uart.h>

#define AM335x_UART0_BASE_ADDRESS    0x44E09000
#define AM335x_UART0_SIZE            4096
#define AM335x_UART0_IRQ             72

#define AM335x_UART1_BASE_ADDRESS    0x48022000
#define AM335x_UART1_SIZE            4096
#define AM335x_UART1_IRQ             73

#define AM335x_UART2_BASE_ADDRESS    0x48024000
#define AM335x_UART2_SIZE            4096
#define AM335x_UART2_IRQ             74

#define AM335x_UART3_BASE_ADDRESS    0x481A6000
#define AM335x_UART3_SIZE            4096
#define AM335x_UART3_IRQ             44

#define AM335x_UART4_BASE_ADDRESS    0x481A8000
#define AM335x_UART4_SIZE            4096
#define AM335x_UART4_IRQ             45

#define AM335x_UART5_BASE_ADDRESS    0x481AA000
#define AM335x_UART5_SIZE            4096
#define AM335x_UART5_IRQ             46

#endif

/**
 * @}
 */
