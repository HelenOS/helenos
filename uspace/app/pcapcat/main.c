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

#define NAME "pcapcat"

typedef struct {
    uint32_t linktype;
    void (*parse)(const char *);
} linktype_parser_t;



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

    while (read_bytes > 0)
    {
        if (read_bytes < sizeof(pcap_packet_header_t)) {
            printf("Error: Could not read enough bytes (read %zu bytes)\n", read_bytes);
            return;
        }
        printf("0x%x: %d, %d\n", hdr.magic_stamp, hdr.captured_length, hdr.original_length);
        void *data = malloc(hdr.captured_length);
        fread(data, 1, (size_t)hdr.captured_length, f);
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