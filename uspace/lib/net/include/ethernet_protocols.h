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

/** @addtogroup eth
 *  @{
 */

/** @file
 *  Ethernet protocol numbers according to the on-line IANA - Ethernet numbers - <http://www.iana.org/assignments/ethernet-numbers>, cited January 17 2009.
 */

#ifndef __NET_ETHERNET_PROTOCOLS_H__
#define __NET_ETHERNET_PROTOCOLS_H__

#include <sys/types.h>

/** Ethernet protocol type definition.
 */
typedef uint16_t	eth_type_t;

/** @name Ethernet protocols definitions
 */
/*@{*/

/** Ethernet minimal protocol number.
 *  According to the IEEE 802.3 specification.
 */
#define ETH_MIN_PROTO	0x0600 /*1536*/

/** Ethernet loopback packet protocol type.
 */
#define ETH_P_LOOP		0x0060

/** XEROX PUP (see 0A00) ethernet protocol type.
 */
#define ETH_P_PUP		0x0200

/** PUP Addr Trans (see 0A01) ethernet protocol type.
 */
#define ETH_P_PUPAT		0x0201

/** Nixdorf ethernet protocol type.
 */
#define ETH_P_Nixdorf		0x0400

/** XEROX NS IDP ethernet protocol type.
 */
#define ETH_P_XEROX_NS_IDP		0x0600

/** DLOG ethernet protocol type.
 */
#define ETH_P_DLOG		0x0660

/** DLOG ethernet protocol type.
 */
#define ETH_P_DLOG2		0x0661

/** Internet IP (IPv4) ethernet protocol type.
 */
#define ETH_P_IP		0x0800

/** X.75 Internet ethernet protocol type.
 */
#define ETH_P_X_75		0x0801

/** NBS Internet ethernet protocol type.
 */
#define ETH_P_NBS		0x0802

/** ECMA Internet ethernet protocol type.
 */
#define ETH_P_ECMA		0x0803

/** Chaosnet ethernet protocol type.
 */
#define ETH_P_Chaosnet		0x0804

/** X.25 Level 3 ethernet protocol type.
 */
#define ETH_P_X25		0x0805

/** ARP ethernet protocol type.
 */
#define ETH_P_ARP		0x0806

/** XNS Compatability ethernet protocol type.
 */
#define ETH_P_XNS_Compatability		0x0807

/** Frame Relay ARP ethernet protocol type.
 */
#define ETH_P_Frame_Relay_ARP		0x0808

/** Symbolics Private ethernet protocol type.
 */
#define ETH_P_Symbolics_Private		0x081C

/** Xyplex ethernet protocol type.
 */
#define ETH_P_Xyplex_MIN		0x0888

/** Xyplex ethernet protocol type.
 */
#define ETH_P_Xyplex_MAX		0x088A

/** Ungermann-Bass net debugr ethernet protocol type.
 */
#define ETH_P_Ungermann_Bass_net_debugr		0x0900

/** Xerox IEEE802.3 PUP ethernet protocol type.
 */
#define ETH_P_IEEEPUP		0x0A00

/** PUP Addr Trans ethernet protocol type.
 */
#define ETH_P_IEEEPUPAT		0x0A01

/** Banyan VINES ethernet protocol type.
 */
#define ETH_P_Banyan_VINES		0x0BAD

/** VINES Loopback ethernet protocol type.
 */
#define ETH_P_VINES_Loopback		0x0BAE

/** VINES Echo ethernet protocol type.
 */
#define ETH_P_VINES_Echo		0x0BAF

/** Berkeley Trailer nego ethernet protocol type.
 */
#define ETH_P_Berkeley_Trailer_nego		0x1000

/** Berkeley Trailer encap/IP ethernet protocol type.
 */
#define ETH_P_Berkeley_Trailer_encapIP_MIN		0x1001

/** Berkeley Trailer encap/IP ethernet protocol type.
 */
#define ETH_P_Berkeley_Trailer_encapIP_MAX		0x100F

/** Valid Systems ethernet protocol type.
 */
#define ETH_P_Valid_Systems		0x1600

/** PCS Basic Block Protocol ethernet protocol type.
 */
#define ETH_P_PCS_Basic_Block_Protocol		0x4242

/** BBN Simnet ethernet protocol type.
 */
#define ETH_P_BBN_Simnet		0x5208

/** DEC Unassigned (Exp.) ethernet protocol type.
 */
#define ETH_P_DEC		0x6000

/** DEC MOP Dump/Load ethernet protocol type.
 */
#define ETH_P_DNA_DL		0x6001

/** DEC MOP Remote Console ethernet protocol type.
 */
#define ETH_P_DNA_RC		0x6002

/** DEC DECNET Phase IV Route ethernet protocol type.
 */
#define ETH_P_DNA_RT		0x6003

/** DEC LAT ethernet protocol type.
 */
#define ETH_P_LAT		0x6004

/** DEC Diagnostic Protocol ethernet protocol type.
 */
#define ETH_P_DIAG		0x6005

/** DEC Customer Protocol ethernet protocol type.
 */
#define ETH_P_CUST		0x6006

/** DEC LAVC, SCA ethernet protocol type.
 */
#define ETH_P_SCA		0x6007

/** DEC Unassigned ethernet protocol type.
 */
#define ETH_P_DEC_Unassigned_MIN		0x6008

/** DEC Unassigned ethernet protocol type.
 */
#define ETH_P_DEC_Unassigned_MAX		0x6009

/** Com Corporation ethernet protocol type.
 */
#define ETH_P_Com_Corporation_MIN		0x6010

/** Com Corporation ethernet protocol type.
 */
#define ETH_P_Com_Corporation_MAX		0x6014

/** Trans Ether Bridging ethernet protocol type.
 */
#define ETH_P_Trans_Ether_Bridging		0x6558

/** Raw Frame Relay ethernet protocol type.
 */
#define ETH_P_Raw_Frame_Relay		0x6559

/** Ungermann-Bass download ethernet protocol type.
 */
#define ETH_P_Ungermann_Bass_download		0x7000

/** Ungermann-Bass dia/loop ethernet protocol type.
 */
#define ETH_P_Ungermann_Bass_dialoop		0x7002

/** LRT ethernet protocol type.
 */
#define ETH_P_LRT_MIN		0x7020

/** LRT ethernet protocol type.
 */
#define ETH_P_LRT_MAX		0x7029

/** Proteon ethernet protocol type.
 */
#define ETH_P_Proteon		0x7030

/** Cabletron ethernet protocol type.
 */
#define ETH_P_Cabletron		0x7034

/** Cronus VLN ethernet protocol type.
 */
#define ETH_P_Cronus_VLN		0x8003

/** Cronus Direct ethernet protocol type.
 */
#define ETH_P_Cronus_Direct		0x8004

/** HP Probe ethernet protocol type.
 */
#define ETH_P_HP_Probe		0x8005

/** Nestar ethernet protocol type.
 */
#define ETH_P_Nestar		0x8006

/** AT&T ethernet protocol type.
 */
#define ETH_P_AT_T		0x8008

/** Excelan ethernet protocol type.
 */
#define ETH_P_Excelan		0x8010

/** SGI diagnostics ethernet protocol type.
 */
#define ETH_P_SGI_diagnostics		0x8013

/** SGI network games ethernet protocol type.
 */
#define ETH_P_SGI_network_games		0x8014

/** SGI reserved ethernet protocol type.
 */
#define ETH_P_SGI_reserved		0x8015

/** SGI bounce server ethernet protocol type.
 */
#define ETH_P_SGI_bounce_server		0x8016

/** Apollo Domain ethernet protocol type.
 */
#define ETH_P_Apollo_Domain		0x8019

/** Tymshare ethernet protocol type.
 */
#define ETH_P_Tymshare		0x802E

/** Tigan, Inc. ethernet protocol type.
 */
#define ETH_P_Tigan		0x802F

/** Reverse ARP ethernet protocol type.
 */
#define ETH_P_RARP		0x8035

/** Aeonic Systems ethernet protocol type.
 */
#define ETH_P_Aeonic_Systems		0x8036

/** DEC LANBridge ethernet protocol type.
 */
#define ETH_P_DEC_LANBridge		0x8038

/** DEC Unassigned ethernet protocol type.
 */
#define ETH_P_DEC_Unassigned_MIN1		0x8039

/** DEC Unassigned ethernet protocol type.
 */
#define ETH_P_DEC_Unassigned_MAX2		0x803C

/** DEC Ethernet Encryption ethernet protocol type.
 */
#define ETH_P_DEC_Ethernet_Encryption		0x803D

/** DEC Unassigned ethernet protocol type.
 */
#define ETH_P_DEC_Unassigned		0x803E

/** DEC LAN Traffic Monitor ethernet protocol type.
 */
#define ETH_P_DEC_LAN_Traffic_Monitor		0x803F

/** DEC Unassigned ethernet protocol type.
 */
#define ETH_P_DEC_Unassigned_MIN3		0x8040

/** DEC Unassigned ethernet protocol type.
 */
#define ETH_P_DEC_Unassigned_MAX3		0x8042

/** Planning Research Corp. ethernet protocol type.
 */
#define ETH_P_Planning_Research_Corp		0x8044

/** AT&T ethernet protocol type.
 */
#define ETH_P_AT_T2		0x8046

/** AT&T ethernet protocol type.
 */
#define ETH_P_AT_T3		0x8047

/** ExperData ethernet protocol type.
 */
#define ETH_P_ExperData		0x8049

/** Stanford V Kernel exp. ethernet protocol type.
 */
#define ETH_P_Stanford_V_Kernel_exp		0x805B

/** Stanford V Kernel prod. ethernet protocol type.
 */
#define ETH_P_Stanford_V_Kernel_prod		0x805C

/** Evans &Sutherland ethernet protocol type.
 */
#define ETH_P_Evans_Sutherland		0x805D

/** Little Machines ethernet protocol type.
 */
#define ETH_P_Little_Machines		0x8060

/** Counterpoint Computers ethernet protocol type.
 */
#define ETH_P_Counterpoint_Computers		0x8062

/** Univ. of Mass. @ Amherst ethernet protocol type.
 */
#define ETH_P_Univ_of_Mass		0x8065

/** Univ. of Mass. @ Amherst ethernet protocol type.
 */
#define ETH_P_Univ_of_Mass2		0x8066

/** Veeco Integrated Auto. ethernet protocol type.
 */
#define ETH_P_Veeco_Integrated_Auto		0x8067

/** General Dynamics ethernet protocol type.
 */
#define ETH_P_General_Dynamics		0x8068

/** AT&T ethernet protocol type.
 */
#define ETH_P_AT_T4		0x8069

/** Autophon ethernet protocol type.
 */
#define ETH_P_Autophon		0x806A

/** ComDesign ethernet protocol type.
 */
#define ETH_P_ComDesign		0x806C

/** Computgraphic Corp. ethernet protocol type.
 */
#define ETH_P_Computgraphic_Corp		0x806D

/** Landmark Graphics Corp. ethernet protocol type.
 */
#define ETH_P_Landmark_Graphics_Corp_MIN		0x806E

/** Landmark Graphics Corp. ethernet protocol type.
 */
#define ETH_P_Landmark_Graphics_Corp_MAX		0x8077

/** Matra ethernet protocol type.
 */
#define ETH_P_Matra		0x807A

/** Dansk Data Elektronik ethernet protocol type.
 */
#define ETH_P_Dansk_Data_Elektronik		0x807B

/** Merit Internodal ethernet protocol type.
 */
#define ETH_P_Merit_Internodal		0x807C

/** Vitalink Communications ethernet protocol type.
 */
#define ETH_P_Vitalink_Communications_MIN		0x807D

/** Vitalink Communications ethernet protocol type.
 */
#define ETH_P_Vitalink_Communications_MAX		0x807F

/** Vitalink TransLAN III ethernet protocol type.
 */
#define ETH_P_Vitalink_TransLAN_III		0x8080

/** Counterpoint Computers ethernet protocol type.
 */
#define ETH_P_Counterpoint_Computers_MIN		0x8081

/** Counterpoint Computers ethernet protocol type.
 */
#define ETH_P_Counterpoint_Computers_MAX		0x8083

/** Appletalk ethernet protocol type.
 */
#define ETH_P_ATALK		0x809B

/** Datability ethernet protocol type.
 */
#define ETH_P_Datability_MIN		0x809C

/** Datability ethernet protocol type.
 */
#define ETH_P_Datability_MAX		0x809E

/** Spider Systems Ltd. ethernet protocol type.
 */
#define ETH_P_Spider_Systems_Ltd		0x809F

/** Nixdorf Computers ethernet protocol type.
 */
#define ETH_P_Nixdorf_Computers		0x80A3

/** Siemens Gammasonics Inc. ethernet protocol type.
 */
#define ETH_P_Siemens_Gammasonics_Inc_MIN		0x80A4

/** Siemens Gammasonics Inc. ethernet protocol type.
 */
#define ETH_P_Siemens_Gammasonics_Inc_MAX		0x80B3

/** DCA Data Exchange Cluster ethernet protocol type.
 */
#define ETH_P_DCA_Data_Exchange_Cluster_MIN		0x80C0

/** DCA Data Exchange Cluster ethernet protocol type.
 */
#define ETH_P_DCA_Data_Exchange_Cluster_MAX		0x80C3

/** Banyan Systems ethernet protocol type.
 */
#define ETH_P_Banyan_Systems		0x80C4

/** Banyan Systems ethernet protocol type.
 */
#define ETH_P_Banyan_Systems2		0x80C5

/** Pacer Software ethernet protocol type.
 */
#define ETH_P_Pacer_Software		0x80C6

/** Applitek Corporation ethernet protocol type.
 */
#define ETH_P_Applitek_Corporation		0x80C7

/** Intergraph Corporation ethernet protocol type.
 */
#define ETH_P_Intergraph_Corporation_MIN		0x80C8

/** Intergraph Corporation ethernet protocol type.
 */
#define ETH_P_Intergraph_Corporation_MAX		0x80CC

/** Harris Corporation ethernet protocol type.
 */
#define ETH_P_Harris_Corporation_MIN		0x80CD

/** Harris Corporation ethernet protocol type.
 */
#define ETH_P_Harris_Corporation_MAX		0x80CE

/** Taylor Instrument ethernet protocol type.
 */
#define ETH_P_Taylor_Instrument_MIN		0x80CF

/** Taylor Instrument ethernet protocol type.
 */
#define ETH_P_Taylor_Instrument_MAX		0x80D2

/** Rosemount Corporation ethernet protocol type.
 */
#define ETH_P_Rosemount_Corporation_MIN		0x80D3

/** Rosemount Corporation ethernet protocol type.
 */
#define ETH_P_Rosemount_Corporation_MAX		0x80D4

/** IBM SNA Service on Ether ethernet protocol type.
 */
#define ETH_P_IBM_SNA_Service_on_Ether		0x80D5

/** Varian Associates ethernet protocol type.
 */
#define ETH_P_Varian_Associates		0x80DD

/** Integrated Solutions TRFS ethernet protocol type.
 */
#define ETH_P_Integrated_Solutions_TRFS_MIN		0x80DE

/** Integrated Solutions TRFS ethernet protocol type.
 */
#define ETH_P_Integrated_Solutions_TRFS_MAX		0x80DF

/** Allen-Bradley ethernet protocol type.
 */
#define ETH_P_Allen_Bradley_MIN		0x80E0

/** Allen-Bradley ethernet protocol type.
 */
#define ETH_P_Allen_Bradley_MAX		0x80E3

/** Datability ethernet protocol type.
 */
#define ETH_P_Datability_MIN2		0x80E4

/** Datability ethernet protocol type.
 */
#define ETH_P_Datability_MAX2		0x80F0

/** Retix ethernet protocol type.
 */
#define ETH_P_Retix		0x80F2

/** AppleTalk AARP (Kinetics) ethernet protocol type.
 */
#define ETH_P_AARP		0x80F3

/** Kinetics ethernet protocol type.
 */
#define ETH_P_Kinetics_MIN		0x80F4

/** Kinetics ethernet protocol type.
 */
#define ETH_P_Kinetics_MAX		0x80F5

/** Apollo Computer ethernet protocol type.
 */
#define ETH_P_Apollo_Computer		0x80F7

/** Wellfleet Communications ethernet protocol type.
 */
#define ETH_P_Wellfleet_Communications		0x80FF

/** IEEE 802.1Q VLAN-tagged frames (initially Wellfleet) ethernet protocol type.
 */
#define ETH_P_8021Q		0x8100

/** Wellfleet Communications ethernet protocol type.
 */
#define ETH_P_Wellfleet_Communications_MIN		0x8101

/** Wellfleet Communications ethernet protocol type.
 */
#define ETH_P_Wellfleet_Communications_MAX		0x8103

/** Symbolics Private ethernet protocol type.
 */
#define ETH_P_Symbolics_Private_MIN		0x8107

/** Symbolics Private ethernet protocol type.
 */
#define ETH_P_Symbolics_Private_MAX		0x8109

/** Hayes Microcomputers ethernet protocol type.
 */
#define ETH_P_Hayes_Microcomputers		0x8130

/** VG Laboratory Systems ethernet protocol type.
 */
#define ETH_P_VG_Laboratory_Systems		0x8131

/** Bridge Communications ethernet protocol type.
 */
#define ETH_P_Bridge_Communications_MIN		0x8132

/** Bridge Communications ethernet protocol type.
 */
#define ETH_P_Bridge_Communications_MAX		0x8136

/** Novell, Inc. ethernet protocol type.
 */
#define ETH_P_Novell_Inc_MIN		0x8137

/** Novell, Inc. ethernet protocol type.
 */
#define ETH_P_Novell_Inc_MAX		0x8138

/** KTI ethernet protocol type.
 */
#define ETH_P_KTI_MIN		0x8139

/** KTI ethernet protocol type.
 */
#define ETH_P_KTI_MAX		0x813D

/** Logicraft ethernet protocol type.
 */
#define ETH_P_Logicraft		0x8148

/** Network Computing Devices ethernet protocol type.
 */
#define ETH_P_Network_Computing_Devices		0x8149

/** Alpha Micro ethernet protocol type.
 */
#define ETH_P_Alpha_Micro		0x814A

/** SNMP ethernet protocol type.
 */
#define ETH_P_SNMP		0x814C

/** BIIN ethernet protocol type.
 */
#define ETH_P_BIIN		0x814D

/** BIIN ethernet protocol type.
 */
#define ETH_P_BIIN2		0x814E

/** Technically Elite Concept ethernet protocol type.
 */
#define ETH_P_Technically_Elite_Concept		0x814F

/** Rational Corp ethernet protocol type.
 */
#define ETH_P_Rational_Corp		0x8150

/** Qualcomm ethernet protocol type.
 */
#define ETH_P_Qualcomm_MIN		0x8151

/** Qualcomm ethernet protocol type.
 */
#define ETH_P_Qualcomm_MAX		0x8153

/** Computer Protocol Pty Ltd ethernet protocol type.
 */
#define ETH_P_Computer_Protocol_Pty_Ltd_MIN		0x815C

/** Computer Protocol Pty Ltd ethernet protocol type.
 */
#define ETH_P_Computer_Protocol_Pty_Ltd_MAX		0x815E

/** Charles River Data System ethernet protocol type.
 */
#define ETH_P_Charles_River_Data_System_MIN		0x8164

/** Charles River Data System ethernet protocol type.
 */
#define ETH_P_Charles_River_Data_System_MAX		0x8166

/** XTP ethernet protocol type.
 */
#define ETH_P_XTP		0x817D

/** SGI/Time Warner prop. ethernet protocol type.
 */
#define ETH_P_SGITime_Warner_prop		0x817E

/** HIPPI-FP encapsulation ethernet protocol type.
 */
#define ETH_P_HIPPI_FP_encapsulation		0x8180

/** STP, HIPPI-ST ethernet protocol type.
 */
#define ETH_P_STP_HIPPI_ST		0x8181

/** Reserved for HIPPI-6400 ethernet protocol type.
 */
#define ETH_P_Reserved_for_HIPPI_6400		0x8182

/** Reserved for HIPPI-6400 ethernet protocol type.
 */
#define ETH_P_Reserved_for_HIPPI_64002		0x8183

/** Silicon Graphics prop. ethernet protocol type.
 */
#define ETH_P_Silicon_Graphics_prop_MIN		0x8184

/** Silicon Graphics prop. ethernet protocol type.
 */
#define ETH_P_Silicon_Graphics_prop_MAX		0x818C

/** Motorola Computer ethernet protocol type.
 */
#define ETH_P_Motorola_Computer		0x818D

/** Qualcomm ethernet protocol type.
 */
#define ETH_P_Qualcomm_MIN2		0x819A

/** Qualcomm ethernet protocol type.
 */
#define ETH_P_Qualcomm_MAX2		0x81A3

/** ARAI Bunkichi ethernet protocol type.
 */
#define ETH_P_ARAI_Bunkichi		0x81A4

/** RAD Network Devices ethernet protocol type.
 */
#define ETH_P_RAD_Network_Devices_MIN		0x81A5

/** RAD Network Devices ethernet protocol type.
 */
#define ETH_P_RAD_Network_Devices_MAX		0x81AE

/** Xyplex ethernet protocol type.
 */
#define ETH_P_Xyplex_MIN2		0x81B7

/** Xyplex ethernet protocol type.
 */
#define ETH_P_Xyplex_MAX2		0x81B9

/** Apricot Computers ethernet protocol type.
 */
#define ETH_P_Apricot_Computers_MIN		0x81CC

/** Apricot Computers ethernet protocol type.
 */
#define ETH_P_Apricot_Computers_MAX		0x81D5

/** Artisoft ethernet protocol type.
 */
#define ETH_P_Artisoft_MIN		0x81D6

/** Artisoft ethernet protocol type.
 */
#define ETH_P_Artisoft_MAX		0x81DD

/** Polygon ethernet protocol type.
 */
#define ETH_P_Polygon_MIN		0x81E6

/** Polygon ethernet protocol type.
 */
#define ETH_P_Polygon_MAX		0x81EF

/** Comsat Labs ethernet protocol type.
 */
#define ETH_P_Comsat_Labs_MIN		0x81F0

/** Comsat Labs ethernet protocol type.
 */
#define ETH_P_Comsat_Labs_MAX		0x81F2

/** SAIC ethernet protocol type.
 */
#define ETH_P_SAIC_MIN		0x81F3

/** SAIC ethernet protocol type.
 */
#define ETH_P_SAIC_MAX		0x81F5

/** VG Analytical ethernet protocol type.
 */
#define ETH_P_VG_Analytical_MIN		0x81F6

/** VG Analytical ethernet protocol type.
 */
#define ETH_P_VG_Analytical_MAX		0x81F8

/** Quantum Software ethernet protocol type.
 */
#define ETH_P_Quantum_Software_MIN		0x8203

/** Quantum Software ethernet protocol type.
 */
#define ETH_P_Quantum_Software_MAX		0x8205

/** Ascom Banking Systems ethernet protocol type.
 */
#define ETH_P_Ascom_Banking_Systems_MIN		0x8221

/** Ascom Banking Systems ethernet protocol type.
 */
#define ETH_P_Ascom_Banking_Systems_MAX		0x8222

/** Advanced Encryption Syste ethernet protocol type.
 */
#define ETH_P_Advanced_Encryption_Syste_MIN		0x823E

/** Advanced Encryption Syste ethernet protocol type.
 */
#define ETH_P_Advanced_Encryption_Syste_MAX		0x8240

/** Athena Programming ethernet protocol type.
 */
#define ETH_P_Athena_Programming_MIN		0x827F

/** Athena Programming ethernet protocol type.
 */
#define ETH_P_Athena_Programming_MAX		0x8282

/** Charles River Data System ethernet protocol type.
 */
#define ETH_P_Charles_River_Data_System_MIN2		0x8263

/** Charles River Data System ethernet protocol type.
 */
#define ETH_P_Charles_River_Data_System_MAX2		0x826A

/** Inst Ind Info Tech ethernet protocol type.
 */
#define ETH_P_Inst_Ind_Info_Tech_MIN		0x829A

/** Inst Ind Info Tech ethernet protocol type.
 */
#define ETH_P_Inst_Ind_Info_Tech_MAX		0x829B

/** Taurus Controls ethernet protocol type.
 */
#define ETH_P_Taurus_Controls_MIN		0x829C

/** Taurus Controls ethernet protocol type.
 */
#define ETH_P_Taurus_Controls_MAX		0x82AB

/** Walker Richer &Quinn ethernet protocol type.
 */
#define ETH_P_Walker_Richer_Quinn_MIN		0x82AC

/** Walker Richer &Quinn ethernet protocol type.
 */
#define ETH_P_Walker_Richer_Quinn_MAX		0x8693

/** Idea Courier ethernet protocol type.
 */
#define ETH_P_Idea_Courier_MIN		0x8694

/** Idea Courier ethernet protocol type.
 */
#define ETH_P_Idea_Courier_MAX		0x869D

/** Computer Network Tech ethernet protocol type.
 */
#define ETH_P_Computer_Network_Tech_MIN		0x869E

/** Computer Network Tech ethernet protocol type.
 */
#define ETH_P_Computer_Network_Tech_MAX		0x86A1

/** Gateway Communications ethernet protocol type.
 */
#define ETH_P_Gateway_Communications_MIN		0x86A3

/** Gateway Communications ethernet protocol type.
 */
#define ETH_P_Gateway_Communications_MAX		0x86AC

/** SECTRA ethernet protocol type.
 */
#define ETH_P_SECTRA		0x86DB

/** Delta Controls ethernet protocol type.
 */
#define ETH_P_Delta_Controls		0x86DE

/** IPv6 ethernet protocol type.
 */
#define ETH_P_IPV6		0x86DD

/** ATOMIC ethernet protocol type.
 */
#define ETH_P_ATOMIC		0x86DF

/** Landis &Gyr Powers ethernet protocol type.
 */
#define ETH_P_Landis_Gyr_Powers_MIN		0x86E0

/** Landis &Gyr Powers ethernet protocol type.
 */
#define ETH_P_Landis_Gyr_Powers_MAX		0x86EF

/** Motorola ethernet protocol type.
 */
#define ETH_P_Motorola_MIN		0x8700

/** Motorola ethernet protocol type.
 */
#define ETH_P_Motorola_MAX		0x8710

/** TCP/IP Compression ethernet protocol type.
 */
#define ETH_P_TCPIP_Compression		0x876B

/** IP Autonomous Systems ethernet protocol type.
 */
#define ETH_P_IP_Autonomous_Systems		0x876C

/** Secure Data ethernet protocol type.
 */
#define ETH_P_Secure_Data		0x876D

/** PPP ethernet protocol type.
 */
#define ETH_P_PPP		0x880B

/** MPLS ethernet protocol type.
 */
#define ETH_P_MPLS_UC		0x8847

/** MPLS with upstream-assigned label ethernet protocol type.
 */
#define ETH_P_MPLS_MC		0x8848

/** Invisible Software ethernet protocol type.
 */
#define ETH_P_Invisible_Software_MIN		0x8A96

/** Invisible Software ethernet protocol type.
 */
#define ETH_P_Invisible_Software_MAX		0x8A97

/** PPPoE Discovery Stage ethernet protocol type.
 */
#define ETH_P_PPP_DISC		0x8863

/** PPPoE Session Stage ethernet protocol type.
 */
#define ETH_P_PPP_SES		0x8864

/** Loopback ethernet protocol type.
 */
#define ETH_P_Loopback		0x9000

/** Com(Bridge) XNS Sys Mgmt ethernet protocol type.
 */
#define ETH_P_Com_XNS_Sys_Mgmt		0x9001

/** Com(Bridge) TCP-IP Sys ethernet protocol type.
 */
#define ETH_P_Com_TCP_IP_Sys		0x9002

/** Com(Bridge) loop detect ethernet protocol type.
 */
#define ETH_P_Com_loop_detect		0x9003

/** BBN VITAL-LanBridge cache ethernet protocol type.
 */
#define ETH_P_BBN_VITAL_LanBridge_cache		0xFF00

/** ISC Bunker Ramo ethernet protocol type.
 */
#define ETH_P_ISC_Bunker_Ramo_MIN		0xFF00

/** ISC Bunker Ramo ethernet protocol type.
 */
#define ETH_P_ISC_Bunker_Ramo_MAX		0xFF0F

/*@}*/

#endif

/** @}
 */
