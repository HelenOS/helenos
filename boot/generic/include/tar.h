/*
 * Copyright (c) 2013 Vojtech Horky
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

/** @addtogroup libuntar
 * @{
 */
/** @file
 */

#ifndef BOOT_TAR_H_
#define BOOT_TAR_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TAR_BLOCK_SIZE 512

typedef struct tar_header_raw {
	char filename[100];
	char permissions[8];
	char owner[8];
	char group[8];
	char size[12];
	char modification_time[12];
	char checksum[8];
	char type;
	char name[100];
	char ustar_magic[6];
	char ustar_version[2];
	char ustar_owner_name[32];
	char ustar_group_name[32];
	char ustar_device_major[8];
	char ustar_device_minor[8];
	char ustar_prefix[155];
	char ignored[12];
} tar_header_raw_t;

_Static_assert(sizeof(tar_header_raw_t) == TAR_BLOCK_SIZE, "Wrong size for tar header.");

enum {
	TAR_TYPE_NORMAL = '0',
	TAR_TYPE_DIRECTORY = '5',
};

typedef struct {
	const uint8_t *ptr;
	size_t length;
	size_t next;
} tar_t;

bool tar_info(const uint8_t *, const uint8_t *, const char **, size_t *);

#endif

/** @}
 */
