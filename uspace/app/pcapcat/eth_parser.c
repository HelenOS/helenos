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


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <errno.h>
#include <str.h>
#include <io/log.h>
#include "pcap.h"
#include "eth_parser.h"

#define ETH_ADDR_SIZE       6
#define IPV4_ADDR_SIZE      4

#define ETHER_TYPE_ARP      0x0806
#define ETHER_TYPE_IP4      0x0800
#define ETHER_TYPE_IP6      0x86DD

#define BYTE_SIZE           8

#define IP_PROTOCOL_TCP     0x06
#define IP_PROTOCOL_UDP     0x11
#define IP_PROTOCOL_ICMP    0x01

#define TCP_TEXT            "TCP"
#define IP_TEXT             "IP"
#define MAC_TEXT            "MAC"
#define ARP_TEXT            "ARP"
#define IPV4_TEXT           "IPv4"
#define IPV6_TEXT           "IPv6"

#define PRINT_IP(msg, ip_addr, spaces) printf("%s %s: %d.%d.%d.%d%s", msg, IP_TEXT, ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3], spaces)
#define PRINT_MAC(msg, mac, spaces) printf("%s %s: %02x:%02x:%02x:%02x:%02x:%02x%s", msg, MAC_TEXT, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], spaces)
#define BIG_END_16(buffer, idx) buffer[idx] << BYTE_SIZE | buffer[idx + 1]


static void read_from_buffer(unsigned char *buffer, int start_idx, int count, uint8_t *dst)
{
    for (int i = start_idx; i < start_idx + count; ++i) {
        dst[i - start_idx] = buffer[i];
    }
}

static void parse_arp(unsigned char *byte_source, size_t size)
{
    uint8_t sender_mac[ETH_ADDR_SIZE];
    uint8_t sender_ip[IPV4_ADDR_SIZE];
    uint8_t target_mac[ETH_ADDR_SIZE];
    uint8_t target_ip[IPV4_ADDR_SIZE];

    read_from_buffer(byte_source, 22, ETH_ADDR_SIZE, sender_mac);
    read_from_buffer(byte_source, 28, IPV4_ADDR_SIZE, sender_ip);
    read_from_buffer(byte_source, 32, ETH_ADDR_SIZE, target_mac);
    read_from_buffer(byte_source, 36, IPV4_ADDR_SIZE, target_ip);

    PRINT_MAC("Sender", sender_mac, ", ");
    PRINT_IP("Sender", sender_ip, "  ");
    PRINT_MAC("Target", target_mac, ", ");
    PRINT_IP("Target", target_ip, "\n");

}

static void parse_tcp(unsigned char *byte_source, size_t size)
{
    uint16_t src_port = BIG_END_16(byte_source, 34);
    uint16_t dst_port = BIG_END_16(byte_source, 36);
    printf("      [%s] source port: %d, destination port: %d\n", TCP_TEXT, src_port, dst_port);
}

static void parse_ip(unsigned char *byte_source, size_t size, bool verbose)
{
    uint16_t total_length;
    uint8_t header_length;
    uint16_t payload_length;
    uint8_t ip_protocol;
    uint8_t src_ip[IPV4_ADDR_SIZE];
    uint8_t dst_ip[IPV4_ADDR_SIZE];

    header_length = (byte_source[14] & 0xf) * 4;
    total_length = BIG_END_16(byte_source, 16);
    payload_length = total_length - header_length;
    ip_protocol = byte_source[23];

    read_from_buffer(byte_source, 26, IPV4_ADDR_SIZE, src_ip);
    read_from_buffer(byte_source, 28, IPV4_ADDR_SIZE, dst_ip);

    printf("%s header: %dB, payload: %dB, protocol: 0x%x, ", IP_TEXT, header_length, payload_length, ip_protocol);
    PRINT_IP("Source", src_ip, ", ");
    PRINT_IP("Destination", dst_ip, "\n");

    if (verbose && ip_protocol == IP_PROTOCOL_TCP) {
        parse_tcp(byte_source, size);
    }
}

static void parse_eth_packet(void *data, size_t size, bool verbose_flag)
{
    unsigned char* byte_source = (unsigned char*)data;

    uint16_t protocol = BIG_END_16(byte_source, 12);

    switch (protocol){
        case ETHER_TYPE_ARP:
            printf("[%s] ", ARP_TEXT);
            parse_arp(byte_source, size);
            break;
        case ETHER_TYPE_IP4:
            printf("[%s] ", IPV4_TEXT);
            parse_ip(byte_source, size, verbose_flag);
            break;
        case ETHER_TYPE_IP6:
            printf("[%s]\n", IPV6_TEXT);
            break;
        default:
            printf("[0x%x]\n", protocol);
            break;
    }
}

void eth_parse_header(pcap_file_header_t *hdr)
{
    printf("LinkType: %d\n", hdr->additional);
    printf("Magic number:  0x%x\n", hdr->magic_number);
}

void eth_parse_packets(FILE *f, int count, bool verbose_flag)
{
    pcap_packet_header_t hdr;

    size_t read_bytes = fread(&hdr, 1, sizeof(pcap_packet_header_t), f);
    int packet_index = 1;
    while (read_bytes > 0)
    {
        if (read_bytes < sizeof(pcap_packet_header_t)) {
            printf("Error: Could not read enough bytes (read %zu bytes)\n", read_bytes);
            return;
        }

        printf("%04d) ", packet_index++);

        void *data = malloc(hdr.captured_length);
        read_bytes = fread(data, 1, (size_t)hdr.captured_length, f);
        if (read_bytes < (size_t)hdr.captured_length) {
            printf("Error: Could not read enough bytes (read %zu bytes)\n", read_bytes);
            return;
        }
        parse_eth_packet(data, (size_t)hdr.captured_length, verbose_flag);
        free(data);

        //Read first count packets
        if (count != -1 && count == packet_index - 1) {
            return;
        }

        memset(&hdr, 0, sizeof(pcap_packet_header_t));
        read_bytes = fread(&hdr, 1, sizeof(pcap_packet_header_t), f);
    }

    fclose(f);
}