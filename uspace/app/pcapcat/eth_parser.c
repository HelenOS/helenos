/*
 * Copyright (c) 2024 Nataliia Korop
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

/** @addtogroup pcapcat
 * @{
 */
/** @file Implementation of functions for parsing PCAP file of LinkType 1 (LINKTYPE_ETHERNET).
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <str.h>
#include <pcap.h>
#include "eth_parser.h"

#define ETH_ADDR_SIZE       6
#define IPV4_ADDR_SIZE      4
#define TCP_PORT_SIZE       2

#define ETHER_TYPE_ARP      0x0806
#define ETHER_TYPE_IP4      0x0800
#define ETHER_TYPE_IP6      0x86DD

#define BYTE_SIZE           8
#define HDR_SIZE_COEF       4
#define LOWER_4_BITS        0xf

#define IP_PROTOCOL_TCP     0x06
#define IP_PROTOCOL_UDP     0x11
#define IP_PROTOCOL_ICMP    0x01

#define TCP_TEXT            "TCP"
#define IP_TEXT             "IP"
#define MAC_TEXT            "MAC"
#define ARP_TEXT            "ARP"
#define IPV4_TEXT           "IPv4"
#define IPV6_TEXT           "IPv6"
#define MALFORMED_PACKET    "packet is malformed.\n"

#define PRINT_IP(msg, ip_addr, spaces) printf("%s %s: %d.%d.%d.%d%s", msg, IP_TEXT, ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3], spaces)
#define PRINT_MAC(msg, mac, spaces) printf("%s %s: %02x:%02x:%02x:%02x:%02x:%02x%s", msg, MAC_TEXT, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], spaces)
#define BIG_END_16(buffer, idx) buffer[idx] << BYTE_SIZE | buffer[idx + 1]

/** Offsets of interesting fields in packet. */

#define ARP_SENDER_MAC      22
#define ARP_SENDER_IP       28
#define ARP_TARGET_MAC      32
#define ARP_TARGET_IP       38

#define TCP_SRC_PORT        34
#define TCP_DST_PORT        36

#define IP_HEADER_LEN       14
#define IP_TOTAL_LEN        16
#define IP_PROTOCOL         23
#define IP_SRC_ADDR         26
#define IP_DST_ADDR         30

/** Read count bytes from char buffer.
 *  @param buffer       of bytes to read from.
 *  @param start_idx    index of the first byte to read.
 *  @param count        number of byte to read.
 *  @param dst          destination buffer.
 */
static void read_from_buffer(unsigned char *buffer, size_t start_idx, size_t count, uint8_t *dst)
{
	for (size_t i = start_idx; i < start_idx + count; ++i) {
		dst[i - start_idx] = buffer[i];
	}
}

/** Parse ARP packet and print out addresses.
 *  @param buffer   ARP packet.
 *  @param size     Size of the packet.
 */
static void parse_arp(unsigned char *buffer, size_t size)
{
	if (size < ARP_TARGET_IP + IPV4_ADDR_SIZE) {
		printf("%s %s", ARP_TEXT, MALFORMED_PACKET);
		return;
	}

	uint8_t sender_mac[ETH_ADDR_SIZE];
	uint8_t sender_ip[IPV4_ADDR_SIZE];
	uint8_t target_mac[ETH_ADDR_SIZE];
	uint8_t target_ip[IPV4_ADDR_SIZE];

	read_from_buffer(buffer, ARP_SENDER_MAC, ETH_ADDR_SIZE, sender_mac);
	read_from_buffer(buffer, ARP_SENDER_IP, IPV4_ADDR_SIZE, sender_ip);
	read_from_buffer(buffer, ARP_TARGET_MAC, ETH_ADDR_SIZE, target_mac);
	read_from_buffer(buffer, ARP_TARGET_IP, IPV4_ADDR_SIZE, target_ip);

	PRINT_MAC("Sender", sender_mac, ", ");
	PRINT_IP("Sender", sender_ip, "  ");
	PRINT_MAC("Target", target_mac, ", ");
	PRINT_IP("Target", target_ip, "\n");
}

/** Parce TCP and print ports.
 *  @param buffer   TCP segment.
 *  @param size     of the buffer.
 */
static void parse_tcp(unsigned char *buffer, size_t size)
{
	if (size < TCP_DST_PORT + TCP_PORT_SIZE) {
		printf("%s %s\n", TCP_TEXT, MALFORMED_PACKET);
		return;
	}

	uint16_t src_port = BIG_END_16(buffer, TCP_SRC_PORT);
	uint16_t dst_port = BIG_END_16(buffer, TCP_DST_PORT);
	printf("      [%s] source port: %d, destination port: %d\n", TCP_TEXT, src_port, dst_port);
}

/** Parse IP and print interesting parts.
 *  @param buffer   IP packet.
 *  @param size     size of the buffer.
 *  @param verbose  verbosity flag.
 */
static void parse_ip(unsigned char *buffer, size_t size, bool verbose)
{
	uint16_t total_length;
	uint8_t header_length;
	uint16_t payload_length;
	uint8_t ip_protocol;
	uint8_t src_ip[IPV4_ADDR_SIZE];
	uint8_t dst_ip[IPV4_ADDR_SIZE];

	if (size < IP_DST_ADDR + IPV4_ADDR_SIZE) {
		printf("%s %s", IP_TEXT, MALFORMED_PACKET);
		return;
	}

	header_length = (buffer[IP_HEADER_LEN] & LOWER_4_BITS) * HDR_SIZE_COEF;
	total_length = BIG_END_16(buffer, IP_TOTAL_LEN);
	payload_length = total_length - header_length;
	ip_protocol = buffer[IP_PROTOCOL];

	read_from_buffer(buffer, IP_SRC_ADDR, IPV4_ADDR_SIZE, src_ip);
	read_from_buffer(buffer, IP_DST_ADDR, IPV4_ADDR_SIZE, dst_ip);

	printf("%s header: %dB, payload: %dB, protocol: 0x%x, ", IP_TEXT, header_length, payload_length, ip_protocol);
	PRINT_IP("Source", src_ip, ", ");
	PRINT_IP("Destination", dst_ip, "\n");

	if (verbose && ip_protocol == IP_PROTOCOL_TCP) {
		parse_tcp(buffer, size);
	}
}

/** Parse ethernnet frame based on eth_type of the frame.
 *  @param data         Ethernet frame.
 *  @param size         Size of the frame.
 *  @param verbose_flag Verbosity flag.
 */
static void parse_eth_frame(void *data, size_t size, bool verbose_flag)
{
	unsigned char *buffer = (unsigned char *)data;

	size_t eth_type_offset = 12;
	uint16_t protocol = BIG_END_16(buffer, eth_type_offset);

	switch (protocol) {
	case ETHER_TYPE_ARP:
		printf("[%s] ", ARP_TEXT);
		parse_arp(buffer, size);
		break;
	case ETHER_TYPE_IP4:
		printf("[%s] ", IPV4_TEXT);
		parse_ip(buffer, size, verbose_flag);
		break;
	case ETHER_TYPE_IP6:
		printf("[%s]\n", IPV6_TEXT);
		break;
	default:
		printf("[0x%x]\n", protocol);
		break;
	}
}

/** Parse file header of PCAP file.
 *  @param hdr  PCAP header structure.
 */
void eth_parse_header(pcap_file_header_t *hdr)
{
	printf("LinkType: %d\n", hdr->additional);
	printf("Magic number:  0x%x\n", hdr->magic_number);
}

/** Parse PCAP file.
 *  @param pcap_file    file of PCAP format with dumped packets.
 *  @param count        number of packets to be parsed and printed from file (if -1 all packets are printed).
 *  @param verbose_flag verbosity flag.
 */
void eth_parse_frames(FILE *pcap_file, int count, bool verbose_flag)
{
	pcap_packet_header_t hdr;

	size_t read_bytes = fread(&hdr, 1, sizeof(pcap_packet_header_t), pcap_file);
	int packet_index = 1;
	while (read_bytes > 0) {
		if (read_bytes < sizeof(pcap_packet_header_t)) {
			printf("Error: Could not read enough bytes (read %zu bytes)\n", read_bytes);
			return;
		}

		printf("%04d) ", packet_index++);

		void *data = malloc(hdr.captured_length);
		read_bytes = fread(data, 1, (size_t)hdr.captured_length, pcap_file);
		if (read_bytes < (size_t)hdr.captured_length) {
			printf("Error: Could not read enough bytes (read %zu bytes)\n", read_bytes);
			return;
		}
		parse_eth_frame(data, (size_t)hdr.captured_length, verbose_flag);
		free(data);

		//Read first count packets from file.
		if (count != -1 && count == packet_index - 1) {
			return;
		}

		memset(&hdr, 0, sizeof(pcap_packet_header_t));
		read_bytes = fread(&hdr, 1, sizeof(pcap_packet_header_t), pcap_file);
	}
}

/** @}
 */
