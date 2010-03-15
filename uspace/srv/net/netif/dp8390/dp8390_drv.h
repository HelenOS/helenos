/*
 * Copyright (c) 2009 Lukas Mejdrech
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

/** @addtogroup dp8390
 *  @{
 */

/** @file
 *  DP8390 network interface driver interface.
 */

#ifndef __NET_NETIF_DP8390_DRIVER_H__
#define __NET_NETIF_DP8390_DRIVER_H__

#include "dp8390.h"

/** Initializes and/or starts the network interface.
 *  @param[in,out] dep The network interface structure.
 *  @param[in] mode The state mode.
 *  @returns EOK on success.
 *  @returns EXDEV if the network interface is disabled.
 */
int do_init(dpeth_t *dep, int mode);

/** Stops the network interface.
 *  @param[in,out] dep The network interface structure.
 */
void do_stop(dpeth_t *dep);

/** Processes the interrupt.
 *  @param[in,out] dep The network interface structure.
 *  @param[in] isr The interrupt status register.
 */
void dp_check_ints(dpeth_t *dep, int isr);

/** Probes and initializes the network interface.
 *  @param[in,out] dep The network interface structure.
 *  @returns EOK on success.
 *  @returns EXDEV if the network interface was not recognized.
 */
int do_probe(dpeth_t * dep);

/** Sends a packet.
 *  @param[in,out] dep The network interface structure.
 *  @param[in] packet The packet t be sent.
 *  @param[in] from_int The value indicating whether the sending is initialized from the interrupt handler.
 *  @returns 
 */
int do_pwrite(dpeth_t * dep, packet_t packet, int from_int);

/** Prints out network interface information.
 *  @param[in] dep The network interface structure.
 */
void dp8390_dump(dpeth_t * dep);

#endif

/** @}
 */
