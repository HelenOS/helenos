/*
 * Copyright (c) 2012 Jiri Svoboda
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

/** @addtogroup inet
 * @{
 */
/**
 * @file
 * @brief
 */

#ifndef INET_PDU_H_
#define INET_PDU_H_

#include <loc.h>
#include <stddef.h>
#include <stdint.h>
#include "inetsrv.h"
#include "ndp.h"

#define INET_CHECKSUM_INIT 0xffff

extern uint16_t inet_checksum_calc(uint16_t, void *, size_t);

extern errno_t inet_pdu_encode(inet_packet_t *, addr32_t, addr32_t, size_t, size_t,
    void **, size_t *, size_t *);
extern errno_t inet_pdu_encode6(inet_packet_t *, addr128_t, addr128_t, size_t,
    size_t, void **, size_t *, size_t *);
extern errno_t inet_pdu_decode(void *, size_t, service_id_t, inet_packet_t *);
extern errno_t inet_pdu_decode6(void *, size_t, service_id_t, inet_packet_t *);

extern errno_t ndp_pdu_decode(inet_dgram_t *, ndp_packet_t *);
extern errno_t ndp_pdu_encode(ndp_packet_t *, inet_dgram_t *);

#endif

/** @}
 */
