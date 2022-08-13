/*
 * SPDX-FileCopyrightText: 2012 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief Texas Instruments AMDM37x UART driver
 */

#ifndef _AMDM37x_UART_H_
#define _AMDM37x_UART_H_

#include <genarch/drivers/omap/uart.h>

/* AMDM37x TRM p. 2950 */
#define AMDM37x_UART1_BASE_ADDRESS   0x4806a000
#define AMDM37x_UART1_SIZE   1024
#define AMDM37x_UART1_IRQ   72 /* AMDM37x TRM p. 2418 */

#define AMDM37x_UART2_BASE_ADDRESS   0x4806b000
#define AMDM37x_UART2_SIZE   1024
#define AMDM37x_UART2_IRQ   73 /* AMDM37x TRM p. 2418 */

#define AMDM37x_UART3_BASE_ADDRESS   0x49020000
#define AMDM37x_UART3_SIZE   1024
#define AMDM37x_UART3_IRQ   74 /* AMDM37x TRM p. 2418 */

#define AMDM37x_UART4_BASE_ADDRESS   0x49042000
#define AMDM37x_UART4_SIZE   1024
#define AMDM37x_UART4_IRQ   80 /* AMDM37x TRM p. 2418 */

#endif

/**
 * @}
 */
