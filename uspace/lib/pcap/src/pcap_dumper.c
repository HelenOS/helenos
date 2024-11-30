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
/** @file Pcap dumper. Structure is a part of every device that is in category PCAP and can dump packets.
 */

#include <errno.h>
#include <str.h>
#include <io/log.h>
#include "pcap_dumper.h"

#define SHORT_OPS_BYTE_COUNT 0x3C

/** Initialize writing to .pcap file.
 *
 * @param writer    Interface for writing data.
 * @param filename  Name of the file for dumping packets.
 * @return          EOK on success, otherwise an error code.
 *
 */
static errno_t pcap_writer_to_file_init(pcap_writer_t *writer, const char *filename)
{
	/** For overwriting file if already exists. */
	writer->data = fopen(filename, "w");
	if (writer->data == NULL) {
		return EINVAL;
	}
	fclose(writer->data);

	writer->data = fopen(filename, "a");
	if (writer->data == NULL) {
		return EINVAL;
	}
	pcap_writer_add_header(writer, (uint32_t)PCAP_LINKTYPE_ETHERNET, false);

	return EOK;
}

/** Open file for appending to the end of it.
 *  @param writer 	Interface for writing data.
 *  @param filename Path to the file.
 *  @return 		EOK on success, error code otherwise.
 */
static errno_t pcap_writer_to_file_init_append(pcap_writer_t *writer, const char *filename)
{
	writer->data = fopen(filename, "a");
	if (writer->data == NULL) {
		return EINVAL;
	}

	return EOK;
}

/** Initialize file for dumping usb packets.
 *  @param writer 	Interface for writing data.
 *  @param filename Path to the file.
 *  @return 		EOK on success, error code otherwise.
 */
static errno_t pcap_writer_to_file_usb_init(pcap_writer_t *writer, const char *filename)
{
	/** For overwriting file if already exists. */
	writer->data = fopen(filename, "w");
	if (writer->data == NULL) {
		return EINVAL;
	}
	fclose(writer->data);

	writer->data = fopen(filename, "a");
	if (writer->data == NULL) {
		return EINVAL;
	}
	pcap_writer_add_header(writer, (uint32_t)PCAP_LINKTYPE_USB_LINUX_MMAPPED, false);

	return EOK;
}

/** Write 4B to the file.
 *  @param writer 	Interface for writing data.
 *  @param data 	Bytes to write.
 *  @return 		Size of successfully witten data.
 */
static size_t pcap_file_w32(pcap_writer_t *writer, uint32_t data)
{
	return fwrite(&data, 1, 4, (FILE *)writer->data);
}

/** Write 2B to the file.
 *  @param writer 	Interface for writing data.
 *  @param data 	Bytes to write.
 *  @return 		Size of successfully witten data.
 */
static size_t pcap_file_w16(pcap_writer_t *writer, uint16_t data)
{
	return fwrite(&data, 1, 2, (FILE *)writer->data);
}

/** Write block of bytes to the file.
 *  @param writer 	Interface for writing data.
 *  @param data 	Bytes to write.
 *  @param size		Size of block of bytes.
 *  @return 		Size of successfully witten data.
 */
static size_t pcap_file_wbuffer(pcap_writer_t *writer, const void *data, size_t size)
{
	assert(writer->data);
	return fwrite(data, 1, size, (FILE *)writer->data);
}

/** Close file for writing.
 *  @param writer 	Interaface for writing data.
 */
static void pcap_file_close(pcap_writer_t *writer)
{
	fclose((FILE *)writer->data);
	writer->data = NULL;
}

/** Write <= 60B of block of bytes.
 *  @param writer 	Interface for writing data.
 *  @param data 	Bytes to write.
 *  @param size		Size of block of bytes.
 *  @return 		Size of successfully witten data.
 */
static size_t pcap_short_file_wbuffer(pcap_writer_t *writer, const void *data, size_t size)
{
	return fwrite(data, 1, size < SHORT_OPS_BYTE_COUNT ? size : SHORT_OPS_BYTE_COUNT, (FILE *)writer->data);
}

/** Standard writer operations for writing data to a newly created file. */
static const pcap_writer_ops_t file_ops = {
	.open = &pcap_writer_to_file_init,
	.write_u32 = &pcap_file_w32,
	.write_u16 = &pcap_file_w16,
	.write_buffer = &pcap_file_wbuffer,
	.close = &pcap_file_close
};

/** Truncated writer operations. Only first 60 bytes of the packet are written. */
static const pcap_writer_ops_t short_file_ops = {
	.open = &pcap_writer_to_file_init,
	.write_u32 = &pcap_file_w32,
	.write_u16 = &pcap_file_w16,
	.write_buffer = &pcap_short_file_wbuffer,
	.close = &pcap_file_close

};

/** Append writer operations. Instead of creating new file open existing file and append packets. */
static const pcap_writer_ops_t append_file_ops = {
	.open = &pcap_writer_to_file_init_append,
	.write_u32 = &pcap_file_w32,
	.write_u16 = &pcap_file_w16,
	.write_buffer = &pcap_file_wbuffer,
	.close = &pcap_file_close
};

/** USB writer operations. Writing USB packets to the file. */
static const pcap_writer_ops_t usb_file_ops = {
	.open = &pcap_writer_to_file_usb_init,
	.write_u32 = &pcap_file_w32,
	.write_u16 = &pcap_file_w16,
	.write_buffer = &pcap_file_wbuffer,
	.close = &pcap_file_close
};

/** Default array of operations. Must be consistens with constants in /uspace/app/pcapctl/main.c */
static pcap_writer_ops_t ops[4] = { file_ops, short_file_ops, append_file_ops, usb_file_ops };

/** Get number of writer operations in @ref ops */
int pcap_dumper_get_ops_number(void)
{
	return (int)(sizeof(ops) / sizeof(pcap_writer_ops_t));
}

/** Open destination buffer for writing and set flag for dumping.
 *  @param dumper	Structure responsible for dumping packets. Part of the driver.
 *  @param name		Name of the destination buffer to dump packets to.
 *  @return 		EOK if successful, erro code otherwise.
 */
errno_t pcap_dumper_start(pcap_dumper_t *dumper, const char *name)
{
	fibril_mutex_lock(&dumper->mutex);

	errno_t rc = dumper->writer.ops->open(&dumper->writer, name);
	if (rc == EOK) {
		dumper->to_dump = true;
	}

	fibril_mutex_unlock(&dumper->mutex);
	return rc;
}

/** Set writer options for the writer.
 *  @param dumper 	Structure responsible for dumping packets. Part of the driver.
 *  @param index	Index of the writer operations from array @ref ops.
 *  @return 		EOK if successful, erro code otherwise.
 */
errno_t pcap_dumper_set_ops(pcap_dumper_t *dumper, int index)
{
	fibril_mutex_lock(&dumper->mutex);
	errno_t rc = EOK;
	dumper->writer.ops = &ops[index];
	fibril_mutex_unlock(&dumper->mutex);
	return rc;
}

/** Write packet to destination buffer.
 *  @param dumper	Structure responsible for dumping packets. Part of the driver.
 *  @param data		Packet data to write.
 *  @param size		Size of the packet.
 */
void pcap_dumper_add_packet(pcap_dumper_t *dumper, const void *data, size_t size)
{
	fibril_mutex_lock(&dumper->mutex);

	if (!dumper->to_dump) {
		fibril_mutex_unlock(&dumper->mutex);
		return;
	}

	pcap_writer_add_packet(&dumper->writer, data, size);
	fibril_mutex_unlock(&dumper->mutex);
}

/** Close destination buffer for writing and unset flag for dumping.
 *  @param dumper	Structure responsible for dumping packets. Part of the driver.
 */
void pcap_dumper_stop(pcap_dumper_t *dumper)
{
	fibril_mutex_lock(&dumper->mutex);

	/** If want to stop, when already stopped, do nothing */
	if (!dumper->to_dump) {
		fibril_mutex_unlock(&dumper->mutex);
		return;
	}
	dumper->to_dump = false;
	dumper->writer.ops->close(&dumper->writer);
	fibril_mutex_unlock(&dumper->mutex);
}

/** @}
 */
