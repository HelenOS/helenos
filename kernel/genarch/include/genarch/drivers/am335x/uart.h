/*
 * Copyright (c) Maurizio Lombardi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
