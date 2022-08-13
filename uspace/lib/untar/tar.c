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

#include <str.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <assert.h>
#include "private/tar.h"

tar_type_t tar_type_parse(const char type)
{
	switch (type) {
	case '0':
	case 0:
		return TAR_TYPE_NORMAL;
	case '5':
		return TAR_TYPE_DIRECTORY;
	default:
		return TAR_TYPE_UNKNOWN;
	}
}

const char *tar_type_str(tar_type_t type)
{
	switch (type) {
	case TAR_TYPE_UNKNOWN:
		return "unknown";
	case TAR_TYPE_NORMAL:
		return "normal";
	case TAR_TYPE_DIRECTORY:
		return "directory";
	default:
		assert(false && "unexpected tar_type_t enum value");
		return "?";
	}
}

errno_t tar_header_parse(tar_header_t *parsed, const tar_header_raw_t *raw)
{
	if (str_length(raw->filename) == 0) {
		return EEMPTY;
	}

	size_t size;
	errno_t rc = str_size_t(raw->size, NULL, 8, true, &size);
	if (rc != EOK) {
		return rc;
	}
	parsed->size = size;

	str_cpy(parsed->filename, 100, raw->filename);

	parsed->type = tar_type_parse(raw->type);

	return EOK;
}

/** @}
 */
