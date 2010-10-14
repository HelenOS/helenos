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

/** @addtogroup tcp
 *  @{
 */

/** @file
 *  TCP options definitions.
 */

#ifndef __NET_TCP_CODES_H__
#define __NET_TCP_CODES_H__

/** End of list TCP option.
 */
#define TCPOPT_END_OF_LIST				0x0

/** No operation TCP option.
 */
#define TCPOPT_NO_OPERATION				0x1

/** Maximum segment size TCP option.
 */
#define TCPOPT_MAX_SEGMENT_SIZE			0x2

/** Maximum segment size TCP option length.
 */
#define TCPOPT_MAX_SEGMENT_SIZE_LENGTH	4

/** Window scale TCP option.
 */
#define TCPOPT_WINDOW_SCALE				0x3

/** Window scale TCP option length.
 */
#define TCPOPT_WINDOW_SCALE_LENGTH		3

/** Selective acknowledgement permitted TCP option.
 */
#define TCPOPT_SACK_PERMITTED			0x4

/** Selective acknowledgement permitted TCP option length.
 */
#define TCPOPT_SACK_PERMITTED_LENGTH	2

/** Selective acknowledgement TCP option.
 *  Has variable length.
 */
#define TCPOPT_SACK						0x5

/** Timestamp TCP option.
 */
#define TCPOPT_TIMESTAMP				0x8

/** Timestamp TCP option length.
 */
#define TCPOPT_TIMESTAMP_LENGTH			10

#endif

/** @}
 */
