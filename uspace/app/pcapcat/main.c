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
//qemu-system-x86_64 -m 256m  -nic   bridge,id=n1,mac=52:54:00:BE:EF:01,br=br0 -boot order=d -cdrom image.iso  -serial stdio

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <errno.h>
#include <str.h>
#include <io/log.h>
#include "pcap.h"

#define NAME "pcapcat"

typedef struct {
    uint32_t linktype;
    void (*parse)(const char *);
} linktype_parser_t;

#define PRINT_IP(msg, ip_addr, spaces) printf("%s IP: %d.%d.%d.%d%s", msg, ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3], spaces)
#define PRINT_MAC(msg, mac, spaces) printf("%s MAC: %x:%x:%x:%x:%x:%x%s", msg, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], spaces)

static void parse_arp(unsigned char *byte_source, size_t size)
{
    uint8_t sender_mac[6];
    uint8_t sender_ip[4];
    uint8_t target_mac[6];
    uint8_t target_ip[4];

    for (int i = 22; i < 28; ++i) {
        sender_mac[i - 22] = byte_source[i];
    }

    for (int i = 28; i < 32; ++i) {
        sender_ip[i - 28] = byte_source[i];
    }

    for (int i = 32; i < 38; ++i) {
        target_mac[i - 32] = byte_source[i];
    }

    for (int i = 38; i < 42; ++i) {
        target_ip[i - 38] = byte_source[i];
    }

    PRINT_MAC("Sender", sender_mac, ", ");
    PRINT_IP("Sender", sender_ip, "  ");
    PRINT_MAC("Target", target_mac, ", ");
    PRINT_IP("Target", target_ip, "\n");

}

static void parse_tcp(unsigned char *byte_source, size_t size)
{
    uint16_t src_port = byte_source[34] << 8 | byte_source[35];
    uint16_t dst_port = byte_source[36] << 8 | byte_source[37];
    printf("      [TCP] source port: %d, destination port: %d\n", src_port, dst_port);
}

static void parse_ip(unsigned char *byte_source, size_t size)
{
    uint16_t total_length;
    uint8_t header_length;
    uint16_t payload_length;
    uint8_t protocol;
    uint8_t src_ip[4];
    uint8_t dst_ip[4];

    header_length = (byte_source[14] & 0xf) * 4;
    total_length = byte_source[16] << 8 | byte_source[17];
    payload_length = total_length - header_length;
    protocol = byte_source[23];

    for (int i = 26; i < 30; ++i) {
        src_ip[i - 26] = byte_source[i];
    }
    for (int i = 30; i < 34; ++i) {
        dst_ip[i - 30] = byte_source[i];
    }
    printf("IP header: %dB, payload: %dB, protocol: 0x%x, ", header_length, payload_length, protocol);
    PRINT_IP("Source", src_ip, ", ");
    PRINT_IP("Destination", dst_ip, "\n");

    if (protocol == 0x6) {
        parse_tcp(byte_source, size);
    }
}


static void parse_packet(void *data, size_t size)
{
    unsigned char* byte_source = (unsigned char*)data;

    // Read the 12th and 13th bytes (indices 11 and 12)
    unsigned char high_byte = byte_source[12]; // MSB
    unsigned char low_byte = byte_source[13];  // LSB

    // Combine bytes in big-endian order
    uint16_t value = (high_byte << 8) | low_byte;

    switch (value){
        case 0x0806:
            printf("[ARP] ");
            parse_arp(byte_source, size);
            break;
        case 0x0800:
            printf("[IPv4] ");
            parse_ip(byte_source, size);
            break;
        case 0x86DD:
            printf("IPv6\n");
            break;
        default:
            printf("0x%x\n", value);
            break;
    }
    //printf("Ethernet Type of packet: 0x%x\n", hdr->etype_len);
}

static void read_head(FILE *file)
{
    pcap_file_header_t data;
    memset(&data, 0, sizeof(pcap_file_header_t));

    size_t bytesRead = fread(&data, 1, sizeof(pcap_file_header_t), file);
    if (bytesRead < sizeof(pcap_file_header_t)) {
        printf("Error: Could not read enough bytes (read %zu bytes)\n", bytesRead);
        fclose(file);
        return;
    }

    printf("LinkType: %d\n", data.additional);
    printf("Magic number:  0x%x\n", data.magic_number);
    return;
}

static void eth_parse(const char *file_path)
{
    FILE *f = fopen(file_path, "rb");
    if (f == NULL){
        printf("File %s does not exist.\n", file_path);
        return;
    }

    read_head(f);

    pcap_packet_header_t hdr;

    memset(&hdr, 0, sizeof(pcap_packet_header_t));
    size_t read_bytes = fread(&hdr, 1, sizeof(pcap_packet_header_t), f);
    int index = 1;
    while (read_bytes > 0)
    {
        if (read_bytes < sizeof(pcap_packet_header_t)) {
            printf("Error: Could not read enough bytes (read %zu bytes)\n", read_bytes);
            return;
        }
        printf("%04d) ", index);
        index++;
        //printf("0x%x: %d, %d\n", hdr.magic_stamp, hdr.captured_length, hdr.original_length);
        void *data = malloc(hdr.captured_length);
        fread(data, 1, (size_t)hdr.captured_length, f);
        parse_packet(data, (size_t)hdr.captured_length);
        free(data);
        read_bytes = fread(&hdr, 1, sizeof(pcap_packet_header_t), f);
    }

    fclose(f);
}

static const linktype_parser_t eth_parser = {
    .parse = &eth_parse,
    .linktype = PCAP_LINKTYPE_ETHERNET
};


int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        return 1;
    }

    eth_parser.parse(argv[1]);

    return 0;
}