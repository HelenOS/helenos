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
#include <getopt.h>
#include <io/log.h>
#include <pcap.h>

#include "linktype_parser.h"
#include "eth_parser.h"

#define NAME "pcapcat"

static const linktype_parser_t eth_parser = {
    .parse_packets = &eth_parse_frames,
    .parse_file_header = &eth_parse_header,
    .linktype = PCAP_LINKTYPE_ETHERNET
};

static const linktype_parser_t parsers[1] = {eth_parser};

static int parse_file(const char *file_path, int packet_count, bool verbose_flag)
{
    FILE *f = fopen(file_path, "rb");
    if (f == NULL){
        printf("File %s does not exist.\n", file_path);
        return 1;
    }

    pcap_file_header_t hdr;
    memset(&hdr, 0, sizeof(pcap_file_header_t));

    size_t bytes_read = fread(&hdr, 1, sizeof(pcap_file_header_t), f);
    if (bytes_read < sizeof(pcap_file_header_t)) {
        printf("Error: Could not read enough bytes (read %zu bytes)\n", bytes_read);
        fclose(f);
        return 1;
    }

    int parser_count = sizeof(parsers) / sizeof(linktype_parser_t);
    int parser_index = -1;
    for (int i = 0; i < parser_count; ++i) {
        if (parsers[i].linktype == hdr.additional) {
            parser_index = i;
            break;
        }
    }

    if (parser_index == -1) {
        printf("There is no parser for Linktype %d.\n", hdr.additional);
        return 1;
    }

    parsers[parser_index].parse_file_header(&hdr);
    parsers[parser_index].parse_packets(f, packet_count, verbose_flag);
    return 0;
}

static void usage()
{
    printf("HelenOS cat utility for PCAP file format.\n"
    "Can run during dumping process.\n"
    "Usage:\n"
    NAME " <filename>\n"
    "\tPrint all packets from file <filename>.\n"
    NAME " --count= | -c <number> <filename>\n"
    "\tPrint first <number> packets from <filename>.\n"
    NAME " --verbose | -v <filename>\n"
    "\tPrint verbose description (with TCP ports) of packets.\n"
    );
}

static struct option options[] = {
    {"count", required_argument, 0, 'c'},
    {"verbose", no_argument, 0, 'v'},
    {0, 0, 0, 0}
};


int main(int argc, char *argv[])
{
    int ret = 0;
    int idx = 0;
    int count = -1;
    bool verbose = false;
    const char *filename = "";
    if (argc == 1)
    {
        usage();
        return 0;
    }

    while (ret != -1) {
        ret = getopt_long(argc, argv, "c:v", options, &idx);
        switch (ret)
        {
        case 'c':
            count = atoi(optarg);
            break;
        case 'v':
            verbose = true;
            break;
        case '?':
            printf("Unknown option or missing argument.\n");
            return 1;
        default:
            break;
        }
    }

    if (optind < argc) {
        filename = argv[optind];
    }

    int ret_val = parse_file(filename, count, verbose);

    return ret_val;
}