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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "blob.h"
#include "source.h"
#include "print.h"
#include "script.h"
#include "transform.h"
#include "tree.h"

int main(int argc, char *argv[])
{
	int rc;
	if (argc < 3) {
		// {True: {}, -1351: "\"false\"", "true": False, 0: b"..."}
		const char data[] = "'Twas brillig, and the slithy toves";
		bithenge_node_t *node;
		bithenge_node_t *subnodes[8];
		bithenge_new_boolean_node(&subnodes[0], true);
		bithenge_new_simple_internal_node(&subnodes[1], NULL, 0, false);
		bithenge_new_integer_node(&subnodes[2], -1351);
		bithenge_new_string_node(&subnodes[3], "\"false\"", false);
		bithenge_new_string_node(&subnodes[4], "true", false);
		bithenge_new_boolean_node(&subnodes[5], false);
		bithenge_new_integer_node(&subnodes[6], 0);
		bithenge_new_blob_from_data(&subnodes[7], data, sizeof(data));
		bithenge_new_simple_internal_node(&node, subnodes, 4, false);
		bithenge_print_node(BITHENGE_PRINT_PYTHON, node);
		printf("\n");
		bithenge_print_node(BITHENGE_PRINT_JSON, node);
		printf("\n");
		bithenge_node_destroy(node);
	} else {
		bithenge_transform_t *transform;
		rc = bithenge_parse_script(argv[1], &transform);
		if (rc != EOK) {
			printf("Error parsing script: %s\n", str_error(rc));
			return 1;
		}

		bithenge_node_t *node, *node2;
		int rc = bithenge_node_from_source(&node, argv[2]);
		if (rc != EOK) {
			printf("Error creating node from source: %s\n", str_error(rc));
			return 1;
		}

		rc = bithenge_transform_apply(transform, node, &node2);
		if (rc != EOK) {
			printf("Error applying transform: %s\n", str_error(rc));
			return 1;
		}

		bithenge_node_destroy(node);
		bithenge_transform_dec_ref(transform);

		rc = bithenge_print_node(BITHENGE_PRINT_PYTHON, node2);
		if (rc != EOK) {
			printf("Error printing node: %s\n", str_error(rc));
			return 1;
		}
		printf("\n");
	}

	return 0;
}

/** @}
 */
