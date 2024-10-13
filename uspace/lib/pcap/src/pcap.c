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
void pcap_set_time(pcap_packet_header_t *header, bool nano) // maybe without bool nano as nano is in pcapng
{
	struct timespec ts;
	getrealtime(&ts);
	header->seconds_stamp = (uint32_t)ts.tv_sec;
	header->magic_stamp = (uint32_t)ts.tv_nsec / 1000;
}

/** Add pcap file header to the new .pcap file.
 *
 * @param writer
 *
 */
void pcap_writer_add_header(pcap_writer_t *writer)
{
	pcap_file_header_t file_header = { PCAP_MAGIC_MICRO, PCAP_MAJOR_VERSION, PCAP_MINOR_VERSION,
		0x00000000, 0x00000000, (uint32_t)PCAP_SNAP_LEN, (uint32_t)PCAP_LINKTYPE_ETHERNET };
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
	if (!writer->data)
		return;
	pcap_packet_header_t pcap_packet;
	pcap_set_time(&pcap_packet, false);
	pcap_packet.original_length = (uint32_t)size;

	if (PCAP_SNAP_LEN < size) {
		pcap_packet.captured_length = PCAP_SNAP_LEN;
	} else {
		pcap_packet.captured_length = size;
	}
	writer->ops->write_buffer(writer, &pcap_packet, sizeof(pcap_packet));
	writer->ops->write_buffer(writer, captured_packet, pcap_packet.captured_length);

}

/** Initialize writing to .pcap file.
 *
 * @param writer    Interface for working with .pcap file
 * @param filename  Name of the file for dumping packets
 * @return          EOK on success or an error code
 *
 */
errno_t pcap_writer_to_file_init(pcap_writer_t *writer, const char *filename)
{
	errno_t rc;
	printf("File: %s\n", filename);
	writer->data = fopen(filename, "a");
	if (writer->data == NULL) {
		rc = EINVAL;
		return rc;
	}
	pcap_writer_add_header(writer);

	rc = EOK;
	return rc;
}

/** @}
 */
