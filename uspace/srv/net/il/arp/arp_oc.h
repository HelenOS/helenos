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

/** @addtogroup arp
 *  @{
 */

/** @file
 *  ARP operation codes according to the on-line IANA - Address Resolution Protocol (ARP) Parameters - <http://www.iana.org/assignments/arp-parameters/arp-parameters.xml>, cited January 14 2009.
 */

#ifndef __NET_ARP_ARPOP_H__
#define __NET_ARP_ARPOP_H__

/** @name ARP operation codes definitions
 */
/*@{*/

/** REQUEST operation code.
 */
#define ARPOP_REQUEST		1

/** REPLY operation code.
 */
#define ARPOP_REPLY		2

/** Reverse request operation code.
 */
#define ARPOP_RREQUEST		3

/** Reverse reply operation code.
 */
#define ARPOP_RREPLY		4

/** DRARP-Request operation code.
 */
#define ARPOP_DRARP_Request		5

/** DRARP-Reply operation code.
 */
#define ARPOP_DRARP_Reply		6

/** DRARP-Error operation code.
 */
#define ARPOP_DRARP_Error		7

/** InARP-Request operation code.
 */
#define ARPOP_InREQUEST		8

/** InARP-Reply operation code.
 */
#define ARPOP_InREPLY		9

/** ARP-NAK operation code.
 */
#define ARPOP_NAK		10

/** MARS-Request operation code.
 */
#define ARPOP_MARS_Request		11

/** MARS-Multi operation code.
 */
#define ARPOP_MARS_Multi		12

/** MARS-MServ operation code.
 */
#define ARPOP_MARS_MServ		13

/** MARS-Join operation code.
 */
#define ARPOP_MARS_Join		14

/** MARS-Leave operation code.
 */
#define ARPOP_MARS_Leave		15

/** MARS-NAK operation code.
 */
#define ARPOP_MARS_NAK		16

/** MARS-Unserv operation code.
 */
#define ARPOP_MARS_Unserv		17

/** MARS-SJoin operation code.
 */
#define ARPOP_MARS_SJoin		18

/** MARS-SLeave operation code.
 */
#define ARPOP_MARS_SLeave		19

/** MARS-Grouplist-Request operation code.
 */
#define ARPOP_MARS_Grouplist_Request		20

/** MARS-Grouplist-Reply operation code.
 */
#define ARPOP_MARS_Grouplist_Reply		21

/** MARS-Redirect-Map operation code.
 */
#define ARPOP_MARS_Redirect_Map		22

/** MAPOS-UNARP operation code.
 */
#define ARPOP_MAPOS_UNARP		23

/*@}*/

#endif

/** @}
 */
