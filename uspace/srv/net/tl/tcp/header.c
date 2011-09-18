/*
 * Copyright (c) 2011 Jiri Svoboda
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
 * @{
 */

/**
 * @file
 */

#include <byteorder.h>
#include "header.h"
#include "segment.h"
#include "std.h"
#include "tcp_type.h"

void tcp_header_setup(tcp_conn_t *conn, tcp_segment_t *seg, tcp_header_t *hdr)
{
	hdr->src_port = host2uint16_t_be(conn->ident.local.port);
	hdr->dest_port = host2uint16_t_be(conn->ident.foreign.port);
	hdr->seq = 0;
	hdr->ack = 0;
	hdr->doff_flags = 0;
	hdr->window = 0;
	hdr->checksum = 0;
	hdr->urg_ptr = 0;
}

void tcp_phdr_setup(tcp_conn_t *conn, tcp_segment_t *seg, tcp_phdr_t *phdr)
{
	phdr->src_addr = conn->ident.local.addr.ipv4;
	phdr->dest_addr = conn->ident.foreign.addr.ipv4;
	phdr->zero = 0;
	phdr->protocol = 0; /* XXX */

	/* XXX This will only work as long as we don't have any header options */
	phdr->tcp_length = sizeof(tcp_header_t) + tcp_segment_data_len(seg);
}

/**
 * @}
 */
