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
 * Simple program to test Bithenge.
 */

#include <loc.h>
#include <stdio.h>
#include <sys/types.h>
#include "blob.h"
#include "block.h"
#include "file.h"

static void
print_data(const char *data, size_t len)
{
	while (len--)
		printf("%02x ", (uint8_t)(*data++));
	printf("\n");
}

static void
print_blob(bithenge_blob_t *blob)
{
	aoff64_t size;
	bithenge_blob_size(blob, &size);
	printf("Size: %d; ", (int)size);
	char buffer[64];
	size = sizeof(buffer);
	bithenge_blob_read(blob, 0, buffer, &size);
	print_data(buffer, size);
}

int main(int argc, char *argv[])
{
	bithenge_blob_t *blob;

	service_id_t service_id;
	loc_service_get_id("bd/initrd", &service_id, 0);
	bithenge_new_block_blob(&blob, service_id);
	printf("Data from block:bd/initrd: ");
	print_blob(blob);
	bithenge_blob_destroy(blob);

	const char data[] = "'Twas brillig, and the slithy toves";
	bithenge_new_blob_from_data(&blob, data, sizeof(data));
	printf("Data from memory (from_data): ");
	print_blob(blob);
	bithenge_blob_destroy(blob);

	bithenge_new_blob_from_buffer(&blob, data, sizeof(data), false);
	printf("Data from memory (from_buffer): ");
	print_blob(blob);
	bithenge_blob_destroy(blob);

	bithenge_new_file_blob(&blob, "/textdemo");
	printf("Data from file:/textdemo: ");
	print_blob(blob);
	bithenge_blob_destroy(blob);

	bithenge_new_file_blob_from_fd(&blob, 0);
	printf("Data from fd:0: ");
	print_blob(blob);
	bithenge_blob_destroy(blob);

	return 0;
}

/** @}
 */
