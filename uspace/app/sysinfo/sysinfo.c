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
#include <sysinfo.h>
#include <sys/types.h>

static int print_item_val(char *ipath);
static int print_item_data(char *ipath);

static void dump_bytes_hex(char *data, size_t size);
static void dump_bytes_text(char *data, size_t size);

static void print_syntax(void);

int main(int argc, char *argv[])
{
	int rc;
	char *ipath;
	sysinfo_item_val_type_t tag;

	if (argc != 2) {
		print_syntax();
		return 1;
	}

	ipath = argv[1];

	tag = sysinfo_get_val_type(ipath);

	/* Silence warning */
	rc = EOK;

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

static int print_item_val(char *ipath)
{
	sysarg_t value;
	int rc;

	rc = sysinfo_get_value(ipath, &value);
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
	void *data;
	size_t size;

	data = sysinfo_get_data(ipath, &size);
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

static void dump_bytes_hex(char *data, size_t size)
{
	size_t i;

	for (i = 0; i < size; ++i) {
		if (i > 0) putchar(' ');
		printf("0x%02x", (uint8_t) data[i]);
	}
}

static void dump_bytes_text(char *data, size_t size)
{
	wchar_t c;
	size_t offset;

	offset = 0;

	while (offset < size) {
		c = str_decode(data, &offset, size);
		printf("%lc", (wint_t) c);
	}
}


static void print_syntax(void)
{
	printf("Syntax: sysinfo <item_path>\n");
}

/** @}
 */
