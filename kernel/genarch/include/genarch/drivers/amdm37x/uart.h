/*
 * Copyright (c) 2012 Jan Vesely
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
