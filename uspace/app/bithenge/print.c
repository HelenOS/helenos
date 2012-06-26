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
 * Write a tree as JSON or other text formats.
 */

#include <errno.h>
#include <stdio.h>
#include "blob.h"
#include "print.h"
#include "tree.h"

typedef struct {
	bithenge_print_type_t type;
	bool first;
} print_internal_data_t;

static int print_internal_func(bithenge_node_t *key, bithenge_node_t *value, void *data_)
{
	print_internal_data_t *data = (print_internal_data_t *)data_;
	int rc = EOK;
	if (!data->first)
		printf(", ");
	data->first = false;
	bool add_quotes = data->type == BITHENGE_PRINT_JSON
	    && bithenge_node_type(key) != BITHENGE_NODE_STRING;
	if (add_quotes)
		printf("\"");
	rc = bithenge_print_node(data->type, key);
	if (rc != EOK)
		goto end;
	if (add_quotes)
		printf("\"");
	printf(": ");
	rc = bithenge_print_node(data->type, value);
	if (rc != EOK)
		goto end;
end:
	bithenge_node_dec_ref(key);
	bithenge_node_dec_ref(value);
	return rc;
}

static int print_internal(bithenge_print_type_t type, bithenge_node_t *node)
{
	int rc;
	print_internal_data_t data = { type, true };
	printf("{");
	rc = bithenge_node_for_each(node, print_internal_func, &data);
	if (rc != EOK)
		return rc;
	printf("}");
	return EOK;
}

static int print_boolean(bithenge_print_type_t type, bithenge_node_t *node)
{
	bool value = bithenge_boolean_node_value(node);
	switch (type) {
	case BITHENGE_PRINT_PYTHON:
		printf(value ? "True" : "False");
		break;
	case BITHENGE_PRINT_JSON:
		printf(value ? "true" : "false");
		break;
	}
	return EOK;
}

static int print_integer(bithenge_print_type_t type, bithenge_node_t *node)
{
	bithenge_int_t value = bithenge_integer_node_value(node);
	printf("%" BITHENGE_PRId, value);
	return EOK;
}

static int print_string(bithenge_print_type_t type, bithenge_node_t *node)
{
	const char *value = bithenge_string_node_value(node);
	printf("\"");
	for (string_iterator_t i = string_iterator(value); !string_iterator_done(&i); ) {
		wchar_t ch;
		int rc = string_iterator_next(&i, &ch);
		if (rc != EOK)
			return rc;
		if (ch == '"' || ch == '\\') {
			printf("\\%lc", (wint_t) ch);
		} else if (ch <= 0x1f) {
			printf("\\u%04x", (unsigned int) ch);
		} else {
			printf("%lc", (wint_t) ch);
		}
	}
	printf("\"");
	return EOK;
}

static int print_blob(bithenge_print_type_t type, bithenge_node_t *node)
{
	bithenge_blob_t *blob = bithenge_node_as_blob(node);
	aoff64_t pos = 0;
	char buffer[1024];
	aoff64_t size = sizeof(buffer);
	int rc;
	printf(type == BITHENGE_PRINT_PYTHON ? "b\"" : "\"");
	do {
		rc = bithenge_blob_read(blob, pos, buffer, &size);
		if (rc != EOK)
			return rc;
		for (aoff64_t i = 0; i < size; i++)
			printf("\\x%02x", (unsigned int)(uint8_t)buffer[i]);
		pos += size;
	} while (size == sizeof(buffer));
	printf("\"");
	return EOK;
}

/** Print a tree as text.
 * @param type The format to use.
 * @param tree The root node of the tree to print.
 * @return EOK on success or an error code from errno.h. */
int bithenge_print_node(bithenge_print_type_t type, bithenge_node_t *tree)
{
	switch (bithenge_node_type(tree)) {
	case BITHENGE_NODE_INTERNAL:
		return print_internal(type, tree);
	case BITHENGE_NODE_BOOLEAN:
		return print_boolean(type, tree);
	case BITHENGE_NODE_INTEGER:
		return print_integer(type, tree);
	case BITHENGE_NODE_STRING:
		return print_string(type, tree);
	case BITHENGE_NODE_BLOB:
		return print_blob(type, tree);
	}
	return ENOTSUP;
}

/** @}
 */
