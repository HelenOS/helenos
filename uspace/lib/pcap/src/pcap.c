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

#include "pcap.h"

/** Set time in seconds and microseconds for the packet header .
 *
 * @param header Header of the packet to be dumped.
 *
 */
void pcap_set_time(pcap_packet_header_t *header)
{
	struct timespec ts;
	getrealtime(&ts);
	header->seconds_stamp = (uint32_t)ts.tv_sec;
	header->magic_stamp = (uint32_t)ts.tv_nsec;
}

/** Add pcap file header to the new .pcap file.
 *
 * @param writer writer that has destination buffer and ops to write to destination buffer.
 *
 */
void pcap_writer_add_header(pcap_writer_t *writer, uint32_t linktype, bool nano)
{
	uint32_t magic_version = PCAP_MAGIC_MICRO;
	if (nano) {
		magic_version = PCAP_MAGIC_NANO;
	}
	pcap_file_header_t file_header = { magic_version, PCAP_MAJOR_VERSION, PCAP_MINOR_VERSION,
		0x00000000, 0x00000000, (uint32_t)PCAP_SNAP_LEN, linktype };
	writer->ops->write_buffer(writer, &file_header, sizeof(file_header));
}

/** Add packet to the .pcap file.
 *
 * @param writer
 * @param captured_packet Packet to be dumped
 * @param size Size of the captured packet
 *
 */
void pcap_writer_add_packet(pcap_writer_t *writer, const void *captured_packet, size_t size)
{

	pcap_packet_header_t pcap_packet;
	pcap_set_time(&pcap_packet);
	pcap_packet.original_length = size;

	if (PCAP_SNAP_LEN < size) {
		pcap_packet.captured_length = PCAP_SNAP_LEN;
	} else {
		pcap_packet.captured_length = size;
	}
	writer->ops->write_buffer(writer, &pcap_packet, sizeof(pcap_packet));
	writer->ops->write_buffer(writer, captured_packet, pcap_packet.captured_length);

}

/** @}
 */
