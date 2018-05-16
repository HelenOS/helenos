/*
 * Copyright (c) 2012 Sean Bartell
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

/** @addtogroup bithenge
 * @{
 */
/**
 * @file
 * Provide various external sources of data.
 */

#include <errno.h>
#include <stdlib.h>
#include <bithenge/blob.h>
#include <bithenge/file.h>
#include <bithenge/source.h>
#include "common.h"

#ifdef __HELENOS__
#include <loc.h>
#include "helenos/block.h"
#endif

static inline int hex_digit(char digit)
{
	if ('0' <= digit && digit <= '9')
		return digit - '0' + 0x0;
	if ('a' <= digit && digit <= 'f')
		return digit - 'a' + 0xa;
	if ('A' <= digit && digit <= 'F')
		return digit - 'A' + 0xA;
	return -1;
}

static errno_t blob_from_hex(bithenge_node_t **out, const char *hex)
{
	size_t size = str_length(hex);
	if (size % 2)
		return EINVAL;
	size /= 2;
	char *buffer = malloc(size);
	if (!buffer)
		return ENOMEM;
	for (size_t i = 0; i < size; i++) {
		int upper = hex_digit(hex[2 * i + 0]);
		int lower = hex_digit(hex[2 * i + 1]);
		if (upper == -1 || lower == -1) {
			free(buffer);
			return EINVAL;
		}
		buffer[i] = upper << 4 | lower;
	}
	return bithenge_new_blob_from_buffer(out, buffer, size, true);
}

/** Create a node from a source described with a string. For instance,
 * "hex:55aa" will result in a blob node. If there is no colon in the string,
 * it is assumed to be a filename.
 * @param[out] out Stores the created node.
 * @param source Specifies the node to be created.
 * @return EOK on success or an error code from errno.h.
 */
errno_t bithenge_node_from_source(bithenge_node_t **out, const char *source)
{
	if (str_chr(source, ':')) {
		if (!str_lcmp(source, "file:", 5)) {
			// Example: file:/textdemo
			return bithenge_new_file_blob(out, source + 5);
#ifdef __HELENOS__
		} else if (!str_lcmp(source, "block:", 6)) {
			// Example: block:bd/initrd
			service_id_t service_id;
			errno_t rc = loc_service_get_id(source + 6, &service_id, 0);
			if (rc != EOK)
				return rc;
			return bithenge_new_block_blob(out, service_id);
#endif
		} else if (!str_lcmp(source, "hex:", 4)) {
			// Example: hex:04000000
			return blob_from_hex(out, source + 4);
		} else {
			return EINVAL;
		}
	}
	return bithenge_new_file_blob(out, source);
}

/** @}
 */
