/*
 * Copyright (c) 1987,1997, 2006, Vrije Universiteit, Amsterdam, The Netherlands All rights reserved. Redistribution and use of the MINIX 3 operating system in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 * * Neither the name of the Vrije Universiteit nor the names of the software authors or contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 * * Any deviations from these conditions require written permission from the copyright holder in advance
 *
 *
 * Disclaimer
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS, AUTHORS, AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ANY AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Changes:
 *  2009 ported to HelenOS, Lukas Mejdrech
 */

/** @addtogroup dp8390
 *  @{
 */

/** @file
 *  Network interface probe functions.
 */

#ifndef __NET_NETIF_DP8390_CONFIG_H__
#define __NET_NETIF_DP8390_CONFIG_H__

#include "dp8390_port.h"

/*
local.h
*/

/** WDETH switch.
 */
#define ENABLE_WDETH 0

/** NE2000 switch.
 */
#define ENABLE_NE2000 1

/** 3C503 switch.
 */
#define ENABLE_3C503 0

/** PCI support switch.
 */
#define ENABLE_PCI 0

struct dpeth;

/* 3c503.c */
/* * Probes a 3C503 network interface.
 *  @param[in] dep The network interface structure.
 *  @returns 1 if the NE2000 network interface is present.
 *  @returns 0 otherwise.
 */
//_PROTOTYPE(int el2_probe, (struct dpeth*dep)				);

/* ne2000.c */
/** Probes a NE2000 or NE1000 network interface.
 *  @param[in] dep The network interface structure.
 *  @returns 1 if the NE2000 network interface is present.
 *  @returns 0 otherwise.
 */
int ne_probe(struct dpeth * dep);
//_PROTOTYPE(int ne_probe, (struct dpeth *dep)				);
//_PROTOTYPE(void ne_init, (struct dpeth *dep)				);

/* rtl8029.c */
/* * Probes a RTL8029 network interface.
 *  @param[in] dep The network interface structure.
 *  @returns 1 if the NE2000 network interface is present.
 *  @returns 0 otherwise.
 */
//_PROTOTYPE(int rtl_probe, (struct dpeth *dep)				);

/* wdeth.c */
/* * Probes a WDETH network interface.
 *  @param[in] dep The network interface structure.
 *  @returns 1 if the NE2000 network interface is present.
 *  @returns 0 otherwise.
 */
//_PROTOTYPE(int wdeth_probe, (struct dpeth*dep)				);

#endif

/** @}
 */
