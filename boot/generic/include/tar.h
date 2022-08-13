/*
 * SPDX-FileCopyrightText: 2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
