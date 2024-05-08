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

/** @addtogroup libpcap
 * @{
 */
/** @file
 *  @brief pcap inteface: Dumping interface for the device which packets we want to dump
 */

#include <errno.h>
#include "pcap_iface.h"

static size_t pcap_file_w32(pcap_writer_t *writer, uint32_t data)
{
	return fwrite(&data, 1, 4, (FILE *)writer->data);
}

static size_t pcap_file_w16(pcap_writer_t *writer, uint16_t data)
{
	return fwrite(&data, 1, 2, (FILE *)writer->data);
}

static size_t pcap_file_wbuffer(pcap_writer_t *writer, const void *data, size_t size)
{
	return fwrite(data, 1, size, (FILE *)writer->data);
}

static void pcap_file_close(pcap_writer_t *writer)
{
	fclose((FILE *)writer->data);
}

static pcap_writer_ops_t file_ops = {

	.write_u32 = &pcap_file_w32,
	.write_u16 = &pcap_file_w16,
	.write_buffer = &pcap_file_wbuffer,
	.close = &pcap_file_close
};

static pcap_writer_t pcap_writer = {
	.ops = &file_ops,
};

errno_t pcap_init(const char *name)
{
	errno_t rc = pcap_writer_to_file_init(&pcap_writer, name);
	return rc;
}

void pcap_add_packet(const void *data, size_t size)
{
	if (pcap_writer.data == NULL)
		return;
	pcap_writer_add_packet(&pcap_writer, data, size);
}

void pcap_close_file(void)
{
	pcap_writer.ops->close(&pcap_writer);
	pcap_writer.data = NULL;
}

/** Initialize interface for dumping packets
 *
 * @param iface Device dumping interface
 *
 */
errno_t pcap_iface_init(pcap_iface_t *iface)
{
	iface->to_dump = false;
	iface->add_packet = pcap_add_packet;
	iface->init = pcap_init;
	iface->fini = pcap_close_file;

	return EOK;
}

/** @}
 */
