/*
 * Copyright (c) 2023 Nataliia Korop
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

/**
 * @addtogroup libpcap
 * @{
 */
/**
 * @file
 * @brief Headers and functions for .pcap file and packets to be dumped
 */

#ifndef PCAP_H_
#define PCAP_H_

#include <stdint.h>
#include <stdio.h>
#include <str_error.h>
#include <time.h>
#include <stdbool.h>
#include <errno.h>

#define PCAP_MAGIC_MICRO 0xA1B2C3D4
#define PCAP_MAGIC_NANO 0xA1B23C4D
#define PCAP_MAJOR_VERSION 0x0002
#define PCAP_MINOR_VERSION 0x0004
#define PCAP_SNAP_LEN 0x00040000

#define PCAP_LINKTYPE_ETHERNET 1    /* IEEE 802.3 Ethernet */
#define PCAP_LINKTYPE_IP_RAW 101	/* Raw IP packet */
#define PCAP_LINKTYPE_IEEE802_11_RADIO 127
#define WIRESHARK_EX 0xc
#define WIRESHARK_SNAPLEN 0xffff

/** Header of the .pcap file
 */
typedef struct {
	uint32_t magic_number;
	uint16_t major_v;
	uint16_t minor_v;
	uint32_t reserved1;
	uint32_t reserved2;
	uint32_t snaplen;
	uint32_t additional; /** The LinkType and additional information field is in the form */
} pcap_file_header_t;

typedef struct pcap_packet_header {
	uint32_t seconds_stamp;
	uint32_t magic_stamp;
	uint32_t captured_length;
	uint32_t original_length;
} pcap_packet_header_t;

typedef struct pcap_writer pcap_writer_t;

typedef struct {
	size_t (*write_u32)(struct pcap_writer *, uint32_t);
	size_t (*write_u16)(struct pcap_writer *, uint16_t);
	size_t (*write_buffer)(struct pcap_writer *, const void *, size_t);
	void (*close)(struct pcap_writer *);

} pcap_writer_ops_t;

/** Interface for working with .pcap file
 */
typedef struct pcap_writer {
	void *data;
	pcap_writer_ops_t *ops;
} pcap_writer_t;

errno_t pcap_writer_to_file_init(pcap_writer_t *writer, const char *filename);

extern void pcap_writer_add_header(pcap_writer_t *);
extern void pcap_writer_add_packet(
    pcap_writer_t *writer, const void *captured_packet, size_t size);

extern void pcap_set_time(pcap_packet_header_t *header);

#endif

/** @}
 */
