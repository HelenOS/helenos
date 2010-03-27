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

/** @addtogroup net_nil
 *  @{
 */

/** @file
 *  Hardware types according to the on-line IANA - Address Resolution Protocol (ARP) Parameters - <http://www.iana.org/assignments/arp-parameters/arp-parameters.xml>, cited January 14 2009.
 */

#ifndef __NET_HW_TYPES_H__
#define __NET_HW_TYPES_H__

#include <sys/types.h>

/** Network interface layer type type definition.
 */
typedef uint8_t	hw_type_t;

/** @name Network interface layer types definitions
 */
/*@{*/

/** Ethernet (10Mb) hardware type.
 */
#define HW_ETHER		1

/** Experimental Ethernet (3Mb) hardware type.
 */
#define HW_EETHER		2

/** Amateur Radio AX.25 hardware type.
 */
#define HW_AX25		3

/** Proteon ProNET Token Ring hardware type.
 */
#define HW_PRONET		4

/** Chaos hardware type.
 */
#define HW_CHAOS		5

/** IEEE 802 Networks hardware type.
 */
#define HW_IEEE802		6

/** ARCNET hardware type.
 */
#define HW_ARCNET		7

/** Hyperchannel hardware type.
 */
#define HW_Hyperchannel		8

/** Lanstar hardware type.
 */
#define HW_Lanstar		9

/** Autonet Short Address hardware type.
 */
#define HW_ASA		10

/** LocalTalk hardware type.
 */
#define HW_LocalTalk		11

/** LocalNet (IBM PCNet or SYTEK LocalNET) hardware type.
 */
#define HW_LocalNet		12

/** Ultra link hardware type.
 */
#define HW_Ultra_link		13

/** SMDS hardware type.
 */
#define HW_SMDS		14

/** Frame Relay DLCI hardware type.
 */
#define HW_DLCI		15

/** Asynchronous Transmission Mode (ATM) hardware type.
 */
#define HW_ATM		16

/** HDLC hardware type.
 */
#define HW_HDLC		17

/** Fibre Channel hardware type.
 */
#define HW_Fibre_Channel		18

/** Asynchronous Transmission Mode (ATM) hardware type.
 */
#define HW_ATM2		19

/** Serial Line hardware type.
 */
#define HW_Serial_Line		20

/** Asynchronous Transmission Mode (ATM) hardware type.
 */
#define HW_ATM3		21

/** MIL-STD-188-220 hardware type.
 */
#define HW_MIL_STD_188_220		22

/** Metricom hardware type.
 */
#define HW_METRICOM		23

/** IEEE 1394.1995 hardware type.
 */
#define HW_IEEE1394		24

/** MAPOS hardware type.
 */
#define HW_MAPOS		25

/** Twinaxial hardware type.
 */
#define HW_Twinaxial		26

/** EUI-64 hardware type.
 */
#define HW_EUI64		27

/** HIPARP hardware type.
 */
#define HW_HIPARP		28

/** IP and ARP over ISO 7816-3 hardware type.
 */
#define HW_ISO_7816_3		29

/** ARPSec hardware type.
 */
#define HW_ARPSec		30

/** IPsec tunnel hardware type.
 */
#define HW_IPsec_tunnel		31

/** InfiniBand (TM) hardware type.
 */
#define HW_INFINIBAND		32

/** TIA-102 Project 25 Common Air Interface (CAI) hardware type.
 */
#define HW_CAI		33

/** Wiegand Interface hardware type.
 */
#define HW_Wiegand		34

/** Pure IP hardware type.
 */
#define HW_Pure_IP		35

/*@}*/

#endif

/** @}
 */
