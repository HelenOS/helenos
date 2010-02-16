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

/** @addtogroup ip
 *  @{
 */

/** @file
 *  Internet protocol numbers according to the on-line IANA - Assigned Protocol numbers - <http://www.iana.org/assignments/protocol-numbers/protocol-numbers.xml>, cited January 14 2009.
 */

#ifndef __NET_IPPROTOCOLS_H__
#define __NET_IPPROTOCOLS_H__

/** @name IP protocols definitions
 */
/*@{*/

/** IPv6 Hop-by-Hop Option internet protocol number.
 */
#define IPPROTO_HOPOPT		0

/** Internet Control Message internet protocol number.
 */
#define IPPROTO_ICMP		1

/** Internet Group Management internet protocol number.
 */
#define IPPROTO_IGMP		2

/** Gateway-to-Gateway internet protocol number.
 */
#define IPPROTO_GGP		3

/** IP in IP (encapsulation) internet protocol number.
 */
#define IPPROTO_IP		4

/** Stream internet protocol number.
 */
#define IPPROTO_ST		5

/** Transmission Control internet protocol number.
 */
#define IPPROTO_TCP		6

/** CBT internet protocol number.
 */
#define IPPROTO_CBT		7

/** Exterior Gateway Protocol internet protocol number.
 */
#define IPPROTO_EGP		8

/** any private interior gateway             
(used by Cisco for their IGRP) internet protocol number.
 */
#define IPPROTO_IGP		9

/** BBN RCC Monitoring internet protocol number.
 */
#define IPPROTO_BBN_RCC_MON		10

/** Network Voice Protocol internet protocol number.
 */
#define IPPROTO_NVP_II		11

/** PUP internet protocol number.
 */
#define IPPROTO_PUP		12

/** ARGUS internet protocol number.
 */
#define IPPROTO_ARGUS		13

/** EMCON internet protocol number.
 */
#define IPPROTO_EMCON		14

/** Cross Net Debugger internet protocol number.
 */
#define IPPROTO_XNET		15

/** Chaos internet protocol number.
 */
#define IPPROTO_CHAOS		16

/** User Datagram internet protocol number.
 */
#define IPPROTO_UDP		17

/** Multiplexing internet protocol number.
 */
#define IPPROTO_MUX		18

/** DCN Measurement Subsystems internet protocol number.
 */
#define IPPROTO_DCN_MEAS		19

/** Host Monitoring internet protocol number.
 */
#define IPPROTO_HMP		20

/** Packet Radio Measurement internet protocol number.
 */
#define IPPROTO_PRM		21

/** XEROX NS IDP internet protocol number.
 */
#define IPPROTO_XNS_IDP		22

/** Trunk-1 internet protocol number.
 */
#define IPPROTO_TRUNK_1		23

/** Trunk-2 internet protocol number.
 */
#define IPPROTO_TRUNK_2		24

/** Leaf-1 internet protocol number.
 */
#define IPPROTO_LEAF_1		25

/** Leaf-2 internet protocol number.
 */
#define IPPROTO_LEAF_2		26

/** Reliable Data Protocol internet protocol number.
 */
#define IPPROTO_RDP		27

/** Internet Reliable Transaction internet protocol number.
 */
#define IPPROTO_IRTP		28

/** ISO Transport Protocol Class 4 internet protocol number.
 */
#define IPPROTO_ISO_TP4		29

/** Bulk Data Transfer Protocol internet protocol number.
 */
#define IPPROTO_NETBLT		30

/** MFE Network Services Protocol internet protocol number.
 */
#define IPPROTO_MFE_NSP		31

/** MERIT Internodal Protocol internet protocol number.
 */
#define IPPROTO_MERIT_INP		32

/** Datagram Congestion Control Protocol internet protocol number.
 */
#define IPPROTO_DCCP		33

/** Third Party Connect Protocol internet protocol number.
 */
#define IPPROTO_3PC		34

/** Inter-Domain Policy Routing Protocol internet protocol number.
 */
#define IPPROTO_IDPR		35

/** XTP internet protocol number.
 */
#define IPPROTO_XTP		36

/** Datagram Delivery Protocol internet protocol number.
 */
#define IPPROTO_DDP		37

/** IDPR Control Message Transport Proto internet protocol number.
 */
#define IPPROTO_IDPR_CMTP		38

/** TP++ Transport Protocol internet protocol number.
 */
#define IPPROTO_TP		39

/** IL Transport Protocol internet protocol number.
 */
#define IPPROTO_IL		40

/** Ipv6 internet protocol number.
 */
#define IPPROTO_IPV6		41

/** Source Demand Routing Protocol internet protocol number.
 */
#define IPPROTO_SDRP		42

/** Routing Header for IPv6 internet protocol number.
 */
#define IPPROTO_IPv6_Route		43

/** Fragment Header for IPv6 internet protocol number.
 */
#define IPPROTO_IPv6_Frag		44

/** Inter-Domain Routing Protocol internet protocol number.
 */
#define IPPROTO_IDRP		45

/** Reservation Protocol internet protocol number.
 */
#define IPPROTO_RSVP		46

/** General Routing Encapsulation internet protocol number.
 */
#define IPPROTO_GRE		47

/** Dynamic Source Routing Protocol internet protocol number.
 */
#define IPPROTO_DSR		48

/** BNA internet protocol number.
 */
#define IPPROTO_BNA		49

/** Encap Security Payload internet protocol number.
 */
#define IPPROTO_ESP		50

/** Authentication Header internet protocol number.
 */
#define IPPROTO_AH		51

/** Integrated Net Layer Security  TUBA internet protocol number.
 */
#define IPPROTO_I_NLSP		52

/** IP with Encryption internet protocol number.
 */
#define IPPROTO_SWIPE		53

/** NBMA Address Resolution Protocol internet protocol number.
 */
#define IPPROTO_NARP		54

/** IP Mobility internet protocol number.
 */
#define IPPROTO_MOBILE		55

/** Transport Layer Security Protocol        
using Kryptonet key management internet protocol number.
 */
#define IPPROTO_TLSP		56

/** SKIP internet protocol number.
 */
#define IPPROTO_SKIP		57

/** ICMP for IPv6 internet protocol number.
 */
#define IPPROTO_IPv6_ICMP		58

/** No Next Header for IPv6 internet protocol number.
 */
#define IPPROTO_IPv6_NoNxt		59

/** Destination Options for IPv6 internet protocol number.
 */
#define IPPROTO_IPv6_Opts		60

/** Any host internal protocol internet protocol number.
 */
#define IPPROTO_AHIP		61

/** CFTP internet protocol number.
 */
#define IPPROTO_CFTP		62

/** Any local network internet protocol number.
 */
#define IPPROTO_ALN		63

/** SATNET and Backroom EXPAK internet protocol number.
 */
#define IPPROTO_SAT_EXPAK		64

/** Kryptolan internet protocol number.
 */
#define IPPROTO_KRYPTOLAN		65

/** MIT Remote Virtual Disk Protocol internet protocol number.
 */
#define IPPROTO_RVD		66

/** Internet Pluribus Packet Core internet protocol number.
 */
#define IPPROTO_IPPC		67

/** Any distributed file system internet protocol number.
 */
#define IPPROTO_ADFS		68

/** SATNET Monitoring internet protocol number.
 */
#define IPPROTO_SAT_MON		69

/** VISA Protocol internet protocol number.
 */
#define IPPROTO_VISA		70

/** Internet Packet Core Utility internet protocol number.
 */
#define IPPROTO_IPCV		71

/** Computer Protocol Network Executive internet protocol number.
 */
#define IPPROTO_CPNX		72

/** Computer Protocol Heart Beat internet protocol number.
 */
#define IPPROTO_CPHB		73

/** Wang Span Network internet protocol number.
 */
#define IPPROTO_WSN		74

/** Packet Video Protocol internet protocol number.
 */
#define IPPROTO_PVP		75

/** Backroom SATNET Monitoring internet protocol number.
 */
#define IPPROTO_BR_SAT_MON		76

/** SUN ND IPPROTOCOL_Temporary internet protocol number.
 */
#define IPPROTO_SUN_ND		77

/** WIDEBAND Monitoring internet protocol number.
 */
#define IPPROTO_WB_MON		78

/** WIDEBAND EXPAK internet protocol number.
 */
#define IPPROTO_WB_EXPAK		79

/** ISO Internet Protocol internet protocol number.
 */
#define IPPROTO_ISO_IP		80

/** VMTP internet protocol number.
 */
#define IPPROTO_VMTP		81

/** SECURE-VMTP internet protocol number.
 */
#define IPPROTO_SECURE_VMTP		82

/** VINES internet protocol number.
 */
#define IPPROTO_VINES		83

/** TTP internet protocol number.
 */
#define IPPROTO_TTP		84

/** NSFNET-IGP internet protocol number.
 */
#define IPPROTO_NSFNET_IGP		85

/** Dissimilar Gateway Protocol internet protocol number.
 */
#define IPPROTO_DGP		86

/** TCF internet protocol number.
 */
#define IPPROTO_TCF		87

/** EIGRP internet protocol number.
 */
#define IPPROTO_EIGRP		88

/** OSPFIGP internet protocol number.
 */
#define IPPROTO_OSPFIGP		89

/** Sprite RPC Protocol internet protocol number.
 */
#define IPPROTO_Sprite_RPC		90

/** Locus Address Resolution Protocol internet protocol number.
 */
#define IPPROTO_LARP		91

/** Multicast Transport Protocol internet protocol number.
 */
#define IPPROTO_MTP		92

/** AX.25 Frames internet protocol number.
 */
#define IPPROTO_AX25		93

/** IP-within-IP Encapsulation Protocol internet protocol number.
 */
#define IPPROTO_IPIP		94

/** Mobile Internetworking Control Pro. internet protocol number.
 */
#define IPPROTO_MICP		95

/** Semaphore Communications Sec. Pro. internet protocol number.
 */
#define IPPROTO_SCC_SP		96

/** Ethernet-within-IP Encapsulation internet protocol number.
 */
#define IPPROTO_ETHERIP		97

/** Encapsulation Header internet protocol number.
 */
#define IPPROTO_ENCAP		98

/** Any private encryption scheme internet protocol number.
 */
#define IPPROTO_APES		99

/** GMTP internet protocol number.
 */
#define IPPROTO_GMTP		100

/** Ipsilon Flow Management Protocol internet protocol number.
 */
#define IPPROTO_IFMP		101

/** PNNI over IP internet protocol number.
 */
#define IPPROTO_PNNI		102

/** Protocol Independent Multicast internet protocol number.
 */
#define IPPROTO_PIM		103

/** ARIS internet protocol number.
 */
#define IPPROTO_ARIS		104

/** SCPS internet protocol number.
 */
#define IPPROTO_SCPS		105

/** QNX internet protocol number.
 */
#define IPPROTO_QNX		106

/** Active Networks internet protocol number.
 */
#define IPPROTO_AN		107

/** IP Payload Compression Protocol internet protocol number.
 */
#define IPPROTO_IPComp		108

/** Sitara Networks Protocol internet protocol number.
 */
#define IPPROTO_SNP		109

/** Compaq Peer Protocol internet protocol number.
 */
#define IPPROTO_Compaq_Peer		110

/** IPX in IP internet protocol number.
 */
#define IPPROTO_IPX_in_IP		111

/** Virtual Router Redundancy Protocol internet protocol number.
 */
#define IPPROTO_VRRP		112

/** PGM Reliable Transport Protocol internet protocol number.
 */
#define IPPROTO_PGM		113

/** Any 0-hop protocol internet protocol number.
 */
#define IPPROTO_A0HP		114

/** Layer Two Tunneling Protocol internet protocol number.
 */
#define IPPROTO_L2TP		115

/** D-II Data Exchange (DDX) internet protocol number.
 */
#define IPPROTO_DDX		116

/** Interactive Agent Transfer Protocol internet protocol number.
 */
#define IPPROTO_IATP		117

/** Schedule Transfer Protocol internet protocol number.
 */
#define IPPROTO_STP		118

/** SpectraLink Radio Protocol internet protocol number.
 */
#define IPPROTO_SRP		119

/** UTI internet protocol number.
 */
#define IPPROTO_UTI		120

/** Simple Message Protocol internet protocol number.
 */
#define IPPROTO_SMP		121

/** SM internet protocol number.
 */
#define IPPROTO_SM		122

/** Performance Transparency Protocol internet protocol number.
 */
#define IPPROTO_PTP		123

/** ISIS over IPv4 internet protocol number.
 */
#define IPPROTO_ISIS		124

/** FIRE internet protocol number.
 */
#define IPPROTO_FIRE		125

/** Combat Radio Transport Protocol internet protocol number.
 */
#define IPPROTO_CRTP		126

/** Combat Radio User Datagram internet protocol number.
 */
#define IPPROTO_CRUDP		127

/** SSCOPMCE internet protocol number.
 */
#define IPPROTO_SSCOPMCE		128

/** IPLT internet protocol number.
 */
#define IPPROTO_IPLT		129

/** Secure Packet Shield internet protocol number.
 */
#define IPPROTO_SPS		130

/** Private IP Encapsulation within IP internet protocol number.
 */
#define IPPROTO_PIPE		131

/** Stream Control Transmission Protocol internet protocol number.
 */
#define IPPROTO_SCTP		132

/** Fibre Channel internet protocol number.
 */
#define IPPROTO_FC		133

/** RSVP-E2E-IGNORE internet protocol number.
 */
#define IPPROTO_RSVP_E2E_IGNORE		134

/** Mobility Header internet protocol number.
 */
#define IPPROTO_MH		135

/** UDPLite internet protocol number.
 */
#define IPPROTO_UDPLITE		136

/** MPLS-in-IP internet protocol number.
 */
#define IPPROTO_MPLS_in_IP		137

/** MANET Protocols internet protocol number.
 */
#define IPPROTO_manet		138

/** Host Identity Protocol internet protocol number.
 */
#define IPPROTO_HIP		139

/** Raw internet protocol number.
 */
#define IPPROTO_RAW		255

/** Maximum internet protocol number.
 */
#define IPPROTO_MAX		( IPPROTO_RAW + 1 )

/*@}*/

#endif

/** @}
 */
