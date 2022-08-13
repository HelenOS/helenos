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

#ifndef TAR_H_
#define TAR_H_

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

typedef enum tar_type {
	TAR_TYPE_UNKNOWN,
	TAR_TYPE_NORMAL,
	TAR_TYPE_DIRECTORY
} tar_type_t;

typedef struct tar_header {
	char filename[100];
	size_t size;
	tar_type_t type;
} tar_header_t;

extern errno_t tar_header_parse(tar_header_t *, const tar_header_raw_t *);
extern tar_type_t tar_type_parse(const char);
extern const char *tar_type_str(tar_type_t);

#endif

/** @}
 */
