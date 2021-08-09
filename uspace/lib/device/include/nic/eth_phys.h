/*
 * Copyright (c) 2011 Radim Vansa
 * Copyright (c) 2011 Jiri Michalec
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

#ifndef LIBDEVICE_NIC_ETH_PHYS_H
#define LIBDEVICE_NIC_ETH_PHYS_H

#include <stdint.h>

/*
 * Definitions of possible supported physical layers
 */

/* Ethernet physical layers */
#define ETH_OLD          0
#define ETH_10M          1
#define ETH_100M         2
#define ETH_1000M        3
#define ETH_10G          4
#define ETH_40G_100G     5
/* 6, 7 reserved for future use */
#define ETH_PHYS_LAYERS  8

/* < 10Mbs ethernets */
#define ETH_EXPERIMENTAL     0x0001
#define ETH_1BASE5           0x0002
/* 10Mbs ethernets */
#define ETH_10BASE5          0x0001
#define ETH_10BASE2          0x0002
#define ETH_10BROAD36        0x0004
#define ETH_STARLAN_10       0x0008
#define ETH_LATTISNET        0x0010
#define ETH_10BASE_T         0x0020
#define ETH_FOIRL            0x0040
#define ETH_10BASE_FL        0x0080
#define ETH_10BASE_FB        0x0100
#define ETH_10BASE_FP        0x0200
/* 100Mbs (fast) ethernets */
#define ETH_100BASE_TX       0x0001
#define ETH_100BASE_T4       0x0002
#define ETH_100BASE_T2       0x0004
#define ETH_100BASE_FX       0x0008
#define ETH_100BASE_SX       0x0010
#define ETH_100BASE_BX10     0x0020
#define ETH_100BASE_LX10     0x0040
#define ETH_100BASE_VG       0x0080
/* 1000Mbs (gigabit) ethernets */
#define ETH_1000BASE_T       0x0001
#define ETH_1000BASE_TX      0x0002
#define ETH_1000BASE_SX      0x0004
#define ETH_1000BASE_LX      0x0008
#define ETH_1000BASE_LH      0x0010
#define ETH_1000BASE_CX      0x0020
#define ETH_1000BASE_BX10    0x0040
#define ETH_1000BASE_LX10    0x0080
#define ETH_1000BASE_PX10_D  0x0100
#define ETH_1000BASE_PX10_U  0x0200
#define ETH_1000BASE_PX20_D  0x0400
#define ETH_1000BASE_PX20_U  0x0800
#define ETH_1000BASE_ZX      0x1000
#define ETH_1000BASE_KX      0x2000
/* 10Gbs ethernets */
#define ETH_10GBASE_SR       0x0001
#define ETH_10GBASE_LX4      0x0002
#define ETH_10GBASE_LR       0x0004
#define ETH_10GBASE_ER       0x0008
#define ETH_10GBASE_SW       0x0010
#define ETH_10GBASE_LW       0x0020
#define ETH_10GBASE_EW       0x0040
#define ETH_10GBASE_CX4      0x0080
#define ETH_10GBASE_T        0x0100
#define ETH_10GBASE_LRM      0x0200
#define ETH_10GBASE_KX4      0x0400
#define ETH_10GBASE_KR       0x0800
/* 40Gbs and 100Gbs ethernets */
#define ETH_40GBASE_SR4      0x0001
#define ETH_40GBASE_LR4      0x0002
#define ETH_40GBASE_CR4      0x0004
#define ETH_40GBASE_KR4      0x0008
#define ETH_100GBASE_SR10    0x0010
#define ETH_100GBASE_LR4     0x0020
#define ETH_100GBASE_ER4     0x0040
#define ETH_100GBASE_CR10    0x0080

/*
 * Auto-negotiation advertisement options
 */

#define ETH_AUTONEG_10BASE_T_HALF    UINT32_C(0x00000001)
#define ETH_AUTONEG_10BASE_T_FULL    UINT32_C(0x00000002)
#define ETH_AUTONEG_100BASE_TX_HALF  UINT32_C(0x00000004)
#define ETH_AUTONEG_100BASE_T4_HALF  UINT32_C(0x00000008)
#define ETH_AUTONEG_100BASE_T2_HALF  UINT32_C(0x00000010)
#define ETH_AUTONEG_100BASE_TX_FULL  UINT32_C(0x00000020)
#define ETH_AUTONEG_100BASE_T2_FULL  UINT32_C(0x00000040)
#define ETH_AUTONEG_1000BASE_T_HALF  UINT32_C(0x00000080)
#define ETH_AUTONEG_1000BASE_T_FULL  UINT32_C(0x00000100)

/** Symetric pause packet (802.3x standard) */
#define ETH_AUTONEG_PAUSE_SYMETRIC   UINT32_C(0x10000000)
/** Asymetric pause packet (802.3z standard, gigabit ethernet) */
#define ETH_AUTONEG_PAUSE_ASYMETRIC  UINT32_C(0x20000000)

#define ETH_AUTONEG_MODE_MASK      UINT32_C(0x0FFFFFFF)
#define ETH_AUTONEG_FEATURES_MASK  UINT32_C(~(ETH_AUTONEG_MODE_MASK))

#define ETH_AUTONEG_MODES  9

struct eth_autoneg_map {
	uint32_t value;
	const char *name;
};

extern const char *ethernet_names[8][17];
extern const struct eth_autoneg_map ethernet_autoneg_mapping[ETH_AUTONEG_MODES];

#endif
