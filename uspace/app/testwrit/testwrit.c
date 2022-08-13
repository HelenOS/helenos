/*
 * SPDX-FileCopyrightText: 2011 Frantisek Princ
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup testwrit
 * @{
 */

#include <stdio.h>
#include <stddef.h>
#include <str.h>

#define BUF_SIZE  1024

int main(int argc, char *argv[])
{
	char buffer[BUF_SIZE];
	uint64_t iterations, i;
	FILE *file;
	char *file_name;

	/* Prepare some example data */
	memset(buffer, 0xcafebabe, BUF_SIZE);

	if (argc != 3) {
		printf("syntax: testwrit <iterations> <target file>\n");
		return 1;
	}

	char *end;
	iterations = strtoul(argv[1], &end, 10);
	file_name = argv[2];

	/* Open target file */
	file = fopen(file_name, "a");
	if (file == NULL) {
		printf("Failed opening file\n");
		return 1;
	}

	/* Writing loop */
	for (i = 0; i < iterations; ++i) {
		fwrite(buffer, 1, BUF_SIZE, file);
	}

	fclose(file);

	return 0;
}

/**
 * @}
 */
