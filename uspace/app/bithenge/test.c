/*
 * SPDX-FileCopyrightText: 2012 Sean Bartell
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup bithenge
 * @{
 */
/**
 * @file
 * Simple program to test Bithenge.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <bithenge/blob.h>
#include <bithenge/source.h>
#include <bithenge/print.h>
#include <bithenge/script.h>
#include <bithenge/transform.h>
#include <bithenge/tree.h>
#include <bithenge/os.h>

#ifdef __HELENOS__
#include <str_error.h>
#else
#include <string.h>
#define str_error strerror
#endif

int main(int argc, char *argv[])
{
	errno_t rc;
	if (argc < 3) {
		fprintf(stderr, "Usage: %s <script> <source>\n", argv[0]);
		return 1;
	} else {
		bithenge_scope_t *scope = NULL;
		bithenge_transform_t *transform = NULL;
		bithenge_node_t *node = NULL, *node2 = NULL;

		rc = bithenge_scope_new(&scope, NULL);
		if (rc != EOK) {
			printf("Error creating scope: %s\n", str_error(rc));
			scope = NULL;
			goto error;
		}

		rc = bithenge_parse_script(argv[1], &transform);
		if (rc != EOK) {
			printf("Error parsing script: %s\n", str_error(rc));
			transform = NULL;
			goto error;
		}

		rc = bithenge_node_from_source(&node, argv[2]);
		if (rc != EOK) {
			printf("Error creating node from source: %s\n", str_error(rc));
			node = NULL;
			goto error;
		}

		rc = bithenge_transform_apply(transform, scope, node, &node2);
		if (rc != EOK) {
			const char *message = bithenge_scope_get_error(scope);
			printf("Error applying transform: %s\n",
			    message ? message : str_error(rc));
			node2 = NULL;
			goto error;
		}

		bithenge_node_dec_ref(node);
		node = NULL;
		bithenge_transform_dec_ref(transform);
		transform = NULL;

		rc = bithenge_print_node(BITHENGE_PRINT_PYTHON, node2);
		if (rc != EOK) {
			const char *message = bithenge_scope_get_error(scope);
			printf("Error printing node: %s\n",
			    message ? message : str_error(rc));
			goto error;
		}
		bithenge_node_dec_ref(node2);
		node2 = NULL;
		bithenge_scope_dec_ref(scope);
		scope = NULL;
		printf("\n");

		return 0;

	error:
		bithenge_node_dec_ref(node);
		bithenge_node_dec_ref(node2);
		bithenge_transform_dec_ref(transform);
		bithenge_scope_dec_ref(scope);
		return 1;
	}

	return 0;
}

/** @}
 */
