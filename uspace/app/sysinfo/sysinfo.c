/*
 * Copyright (c) 2010 Jiri Svoboda
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

/** @addtogroup sysinfo
 * @{
 */
/** @file sysinfo.c
 * @brief Print value of item from sysinfo tree.
 */

#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <sysinfo.h>
#include <stdlib.h>
#include <str.h>

static void dump_bytes_hex(char *data, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		if (i > 0)
			putchar(' ');
		printf("0x%02x", (uint8_t) data[i]);
	}
}

static void dump_bytes_text(char *data, size_t size)
{
	size_t offset = 0;

	while (offset < size) {
		char32_t c = str_decode(data, &offset, size);
		printf("%lc", (wint_t) c);
	}
}

static errno_t print_item_val(char *ipath)
{
	sysarg_t value;
	errno_t rc = sysinfo_get_value(ipath, &value);
	if (rc != EOK) {
		printf("Error reading item '%s'.\n", ipath);
		return rc;
	}

	printf("%s -> %" PRIu64 " (0x%" PRIx64 ")\n", ipath,
	    (uint64_t) value, (uint64_t) value);

	return EOK;
}

static int print_item_data(char *ipath)
{
	size_t size;
	void *data = sysinfo_get_data(ipath, &size);
	if (data == NULL) {
		printf("Error reading item '%s'.\n", ipath);
		return -1;
	}

	printf("%s -> ", ipath);
	dump_bytes_hex(data, size);
	fputs(" ('", stdout);
	dump_bytes_text(data, size);
	fputs("')\n", stdout);

	return EOK;
}

static int print_item_property(char *ipath, char *iprop)
{
	size_t size;
	void *data = sysinfo_get_property(ipath, iprop, &size);
	if (data == NULL) {
		printf("Error reading property '%s' of item '%s'.\n", iprop,
		    ipath);
		return -1;
	}

	printf("%s property %s -> ", ipath, iprop);
	dump_bytes_hex(data, size);
	fputs(" ('", stdout);
	dump_bytes_text(data, size);
	fputs("')\n", stdout);

	return EOK;
}

static void print_spaces(size_t spaces)
{
	for (size_t i = 0; i < spaces; i++)
		printf(" ");
}

static void print_keys(const char *path, size_t spaces)
{
	size_t size;
	char *keys = sysinfo_get_keys(path, &size);
	if ((keys == NULL) || (size == 0))
		return;

	size_t pos = 0;
	while (pos < size) {
		/* Process each key with sanity checks */
		size_t cur_size = str_nsize(keys + pos, size - pos);
		if (keys[pos + cur_size] != 0)
			break;

		size_t path_size = str_size(path) + cur_size + 2;
		char *cur_path = (char *) malloc(path_size);
		if (cur_path == NULL)
			break;

		size_t length;

		if (path[0] != 0) {
			print_spaces(spaces);
			printf(".%s\n", keys + pos);
			length = str_length(keys + pos) + 1;

			snprintf(cur_path, path_size, "%s.%s", path, keys + pos);
		} else {
			printf("%s\n", keys + pos);
			length = str_length(keys + pos);

			snprintf(cur_path, path_size, "%s", keys + pos);
		}

		print_keys(cur_path, spaces + length);

		free(cur_path);
		pos += cur_size + 1;
	}

	free(keys);
}

int main(int argc, char *argv[])
{
	int rc = 0;

	if (argc < 2) {
		/* Print keys */
		print_keys("", 0);
		return rc;
	}

	char *ipath = argv[1];

	if (argc < 3) {
		sysinfo_item_val_type_t tag = sysinfo_get_val_type(ipath);

		switch (tag) {
		case SYSINFO_VAL_UNDEFINED:
			printf("Error: Sysinfo item '%s' not defined.\n", ipath);
			rc = 2;
			break;
		case SYSINFO_VAL_VAL:
			rc = print_item_val(ipath);
			break;
		case SYSINFO_VAL_DATA:
			rc = print_item_data(ipath);
			break;
		default:
			printf("Error: Sysinfo item '%s' with unknown value type.\n",
			    ipath);
			rc = 2;
			break;
		}

		return rc;
	}

	char *iprop = argv[2];
	rc = print_item_property(ipath, iprop);
	return rc;
}

/** @}
 */
