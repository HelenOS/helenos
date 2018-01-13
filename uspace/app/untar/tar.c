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

/** @addtogroup untar
 * @{
 */
/** @file
 */

#include <str.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <assert.h>

#include "tar.h"

tar_type_t tar_type_parse(const char type) {
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

const char *tar_type_str(tar_type_t type) {
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
	errno_t rc;

	if (str_length(raw->filename) == 0) {
		return EEMPTY;
	}

	size_t size;
	rc = str_size_t(raw->size, NULL, 8, true, &size);
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
