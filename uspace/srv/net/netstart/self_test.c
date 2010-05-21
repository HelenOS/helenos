/*
 * Copyright (c) 2009 Lukas Mejdrech
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

/** @addtogroup net
 * @{
 */

/** @file
 * Networking self-tests implementation.
 *
 */

#include <errno.h>
#include <malloc.h>
#include <stdio.h>

#include <net_checksum.h>
#include <adt/int_map.h>
#include <adt/char_map.h>
#include <adt/generic_char_map.h>
#include <adt/measured_strings.h>
#include <adt/dynamic_fifo.h>

#include "self_test.h"

/** Test the statement, compare the result and evaluate.
 *
 * @param[in] statement The statement to test.
 * @param[in] result    The expected result.
 *
 */
#define TEST(statement, result) \
	do { \
		printf("\n\t%s == %s", #statement, #result); \
		if ((statement) != (result)) { \
			printf("\tfailed\n"); \
			fprintf(stderr, "\nNetwork self-test failed\n"); \
			return EINVAL; \
		} else \
			printf("\tOK"); \
	} while (0)

#define XMALLOC(var, type) \
	do { \
		(var) = (type *) malloc(sizeof(type)); \
		if ((var) == NULL) { \
			fprintf(stderr, "\nMemory allocation error\n"); \
			return ENOMEM; \
		} \
	} while (0)

GENERIC_CHAR_MAP_DECLARE(int_char_map, int);
GENERIC_CHAR_MAP_IMPLEMENT(int_char_map, int);

GENERIC_FIELD_DECLARE(int_field, int);
GENERIC_FIELD_IMPLEMENT(int_field, int);

INT_MAP_DECLARE(int_map, int);
INT_MAP_IMPLEMENT(int_map, int);

/** Self-test start function.
 *
 * Run all self-tests.
 *
 * @returns EOK on success.
 * @returns The first error occurred.
 *
 */
int self_test(void)
{
	printf("Running networking self-tests\n");
	
	printf("\nChar map test");
	char_map_t cm;
	
	TEST(char_map_update(&cm, "ucho", 0, 3), EINVAL);
	TEST(char_map_initialize(&cm), EOK);
	TEST(char_map_exclude(&cm, "bla", 0), CHAR_MAP_NULL);
	TEST(char_map_find(&cm, "bla", 0), CHAR_MAP_NULL);
	TEST(char_map_add(&cm, "bla", 0, 1), EOK);
	TEST(char_map_find(&cm, "bla", 0), 1);
	TEST(char_map_add(&cm, "bla", 0, 10), EEXISTS);
	TEST(char_map_update(&cm, "bla", 0, 2), EOK);
	TEST(char_map_find(&cm, "bla", 0), 2);
	TEST(char_map_update(&cm, "ucho", 0, 2), EOK);
	TEST(char_map_exclude(&cm, "bla", 0), 2);
	TEST(char_map_exclude(&cm, "bla", 0), CHAR_MAP_NULL);
	TEST(char_map_find(&cm, "ucho", 0), 2);
	TEST(char_map_update(&cm, "ucho", 0, 3), EOK);
	TEST(char_map_find(&cm, "ucho", 0), 3);
	TEST(char_map_add(&cm, "blabla", 0, 5), EOK);
	TEST(char_map_find(&cm, "blabla", 0), 5);
	TEST(char_map_add(&cm, "bla", 0, 6), EOK);
	TEST(char_map_find(&cm, "bla", 0), 6);
	TEST(char_map_exclude(&cm, "bla", 0), 6);
	TEST(char_map_find(&cm, "bla", 0), CHAR_MAP_NULL);
	TEST(char_map_find(&cm, "blabla", 0), 5);
	TEST(char_map_add(&cm, "auto", 0, 7), EOK);
	TEST(char_map_find(&cm, "auto", 0), 7);
	TEST(char_map_add(&cm, "kara", 0, 8), EOK);
	TEST(char_map_find(&cm, "kara", 0), 8);
	TEST(char_map_add(&cm, "nic", 0, 9), EOK);
	TEST(char_map_find(&cm, "nic", 0), 9);
	TEST(char_map_find(&cm, "blabla", 0), 5);
	TEST(char_map_add(&cm, "micnicnic", 5, 9), EOK);
	TEST(char_map_find(&cm, "micni", 0), 9);
	TEST(char_map_find(&cm, "micnicn", 5), 9);
	TEST(char_map_add(&cm, "\x10\x0\x2\x2", 4, 15), EOK);
	TEST(char_map_find(&cm, "\x10\x0\x2\x2", 4), 15);
	
	TEST((char_map_destroy(&cm), EOK), EOK);
	TEST(char_map_update(&cm, "ucho", 0, 3), EINVAL);
	
	printf("\nCRC computation test");
	uint32_t value;
	
	TEST(value = ~compute_crc32(~0, "123456789", 8 * 9), 0xcbf43926);
	TEST(value = ~compute_crc32(~0, "1", 8), 0x83dcefb7);
	TEST(value = ~compute_crc32(~0, "12", 8 * 2), 0x4f5344cd);
	TEST(value = ~compute_crc32(~0, "123", 8 * 3), 0x884863d2);
	TEST(value = ~compute_crc32(~0, "1234", 8 * 4), 0x9be3e0a3);
	TEST(value = ~compute_crc32(~0, "12345678", 8 * 8), 0x9ae0daaf);
	TEST(value = ~compute_crc32(~0, "ahoj pane", 8 * 9), 0x5fc3d706);
	
	printf("\nDynamic fifo test");
	dyn_fifo_t fifo;
	
	TEST(dyn_fifo_push(&fifo, 1, 0), EINVAL);
	TEST(dyn_fifo_initialize(&fifo, 1), EOK);
	TEST(dyn_fifo_push(&fifo, 1, 0), EOK);
	TEST(dyn_fifo_pop(&fifo), 1);
	TEST(dyn_fifo_pop(&fifo), ENOENT);
	TEST(dyn_fifo_push(&fifo, 2, 1), EOK);
	TEST(dyn_fifo_push(&fifo, 3, 1), ENOMEM);
	TEST(dyn_fifo_push(&fifo, 3, 0), EOK);
	TEST(dyn_fifo_pop(&fifo), 2);
	TEST(dyn_fifo_pop(&fifo), 3);
	TEST(dyn_fifo_push(&fifo, 4, 2), EOK);
	TEST(dyn_fifo_push(&fifo, 5, 2), EOK);
	TEST(dyn_fifo_push(&fifo, 6, 2), ENOMEM);
	TEST(dyn_fifo_push(&fifo, 6, 5), EOK);
	TEST(dyn_fifo_push(&fifo, 7, 5), EOK);
	TEST(dyn_fifo_pop(&fifo), 4);
	TEST(dyn_fifo_pop(&fifo), 5);
	TEST(dyn_fifo_push(&fifo, 8, 5), EOK);
	TEST(dyn_fifo_push(&fifo, 9, 5), EOK);
	TEST(dyn_fifo_push(&fifo, 10, 6), EOK);
	TEST(dyn_fifo_push(&fifo, 11, 6), EOK);
	TEST(dyn_fifo_pop(&fifo), 6);
	TEST(dyn_fifo_pop(&fifo), 7);
	TEST(dyn_fifo_push(&fifo, 12, 6), EOK);
	TEST(dyn_fifo_push(&fifo, 13, 6), EOK);
	TEST(dyn_fifo_push(&fifo, 14, 6), ENOMEM);
	TEST(dyn_fifo_push(&fifo, 14, 8), EOK);
	TEST(dyn_fifo_pop(&fifo), 8);
	TEST(dyn_fifo_pop(&fifo), 9);
	TEST(dyn_fifo_pop(&fifo), 10);
	TEST(dyn_fifo_pop(&fifo), 11);
	TEST(dyn_fifo_pop(&fifo), 12);
	TEST(dyn_fifo_pop(&fifo), 13);
	TEST(dyn_fifo_pop(&fifo), 14);
	TEST(dyn_fifo_destroy(&fifo), EOK);
	TEST(dyn_fifo_push(&fifo, 1, 0), EINVAL);
	
	printf("\nGeneric char map test");
	
	int *x;
	int *y;
	int *z;
	int *u;
	int *v;
	int *w;
	
	XMALLOC(x, int);
	XMALLOC(y, int);
	XMALLOC(z, int);
	XMALLOC(u, int);
	XMALLOC(v, int);
	XMALLOC(w, int);
	
	int_char_map_t icm;
	icm.magic = 0;
	
	TEST(int_char_map_add(&icm, "ucho", 0, z), EINVAL);
	TEST(int_char_map_initialize(&icm), EOK);
	TEST((int_char_map_exclude(&icm, "bla", 0), EOK), EOK);
	TEST(int_char_map_find(&icm, "bla", 0), NULL);
	TEST(int_char_map_add(&icm, "bla", 0, x), EOK);
	TEST(int_char_map_find(&icm, "bla", 0), x);
	TEST(int_char_map_add(&icm, "bla", 0, y), EEXISTS);
	TEST((int_char_map_exclude(&icm, "bla", 0), EOK), EOK);
	TEST((int_char_map_exclude(&icm, "bla", 0), EOK), EOK);
	TEST(int_char_map_add(&icm, "blabla", 0, v), EOK);
	TEST(int_char_map_find(&icm, "blabla", 0), v);
	TEST(int_char_map_add(&icm, "bla", 0, w), EOK);
	TEST(int_char_map_find(&icm, "bla", 0), w);
	TEST((int_char_map_exclude(&icm, "bla", 0), EOK), EOK);
	TEST(int_char_map_find(&icm, "bla", 0), NULL);
	TEST(int_char_map_find(&icm, "blabla", 0), v);
	TEST(int_char_map_add(&icm, "auto", 0, u), EOK);
	TEST(int_char_map_find(&icm, "auto", 0), u);
	TEST((int_char_map_destroy(&icm), EOK), EOK);
	TEST(int_char_map_add(&icm, "ucho", 0, z), EINVAL);
	
	printf("\nGeneric field test");
	
	XMALLOC(x, int);
	XMALLOC(y, int);
	XMALLOC(z, int);
	XMALLOC(u, int);
	XMALLOC(v, int);
	XMALLOC(w, int);
	
	int_field_t gf;
	gf.magic = 0;
	
	TEST(int_field_add(&gf, x), EINVAL);
	TEST(int_field_count(&gf), -1);
	TEST(int_field_initialize(&gf), EOK);
	TEST(int_field_count(&gf), 0);
	TEST(int_field_get_index(&gf, 1), NULL);
	TEST(int_field_add(&gf, x), 0);
	TEST(int_field_get_index(&gf, 0), x);
	TEST((int_field_exclude_index(&gf, 0), EOK), EOK);
	TEST(int_field_get_index(&gf, 0), NULL);
	TEST(int_field_add(&gf, y), 1);
	TEST(int_field_get_index(&gf, 1), y);
	TEST(int_field_add(&gf, z), 2);
	TEST(int_field_get_index(&gf, 2), z);
	TEST(int_field_get_index(&gf, 1), y);
	TEST(int_field_count(&gf), 3);
	TEST(int_field_add(&gf, u), 3);
	TEST(int_field_get_index(&gf, 3), u);
	TEST(int_field_add(&gf, v), 4);
	TEST(int_field_get_index(&gf, 4), v);
	TEST(int_field_add(&gf, w), 5);
	TEST(int_field_get_index(&gf, 5), w);
	TEST(int_field_count(&gf), 6);
	TEST((int_field_exclude_index(&gf, 1), EOK), EOK);
	TEST(int_field_get_index(&gf, 1), NULL);
	TEST(int_field_get_index(&gf, 3), u);
	TEST((int_field_exclude_index(&gf, 7), EOK), EOK);
	TEST(int_field_get_index(&gf, 3), u);
	TEST(int_field_get_index(&gf, 5), w);
	TEST((int_field_exclude_index(&gf, 4), EOK), EOK);
	TEST(int_field_get_index(&gf, 4), NULL);
	TEST((int_field_destroy(&gf), EOK), EOK);
	TEST(int_field_count(&gf), -1);
	
	printf("\nInt map test");
	
	XMALLOC(x, int);
	XMALLOC(y, int);
	XMALLOC(z, int);
	XMALLOC(u, int);
	XMALLOC(v, int);
	XMALLOC(w, int);
	
	int_map_t im;
	im.magic = 0;
	
	TEST(int_map_add(&im, 1, x), EINVAL);
	TEST(int_map_count(&im), -1);
	TEST(int_map_initialize(&im), EOK);
	TEST(int_map_count(&im), 0);
	TEST(int_map_find(&im, 1), NULL);
	TEST(int_map_add(&im, 1, x), 0);
	TEST(int_map_find(&im, 1), x);
	TEST((int_map_exclude(&im, 1), EOK), EOK);
	TEST(int_map_find(&im, 1), NULL);
	TEST(int_map_add(&im, 1, y), 1);
	TEST(int_map_find(&im, 1), y);
	TEST(int_map_add(&im, 4, z), 2);
	TEST(int_map_get_index(&im, 2), z);
	TEST(int_map_find(&im, 4), z);
	TEST(int_map_find(&im, 1), y);
	TEST(int_map_count(&im), 3);
	TEST(int_map_add(&im, 2, u), 3);
	TEST(int_map_find(&im, 2), u);
	TEST(int_map_add(&im, 3, v), 4);
	TEST(int_map_find(&im, 3), v);
	TEST(int_map_get_index(&im, 4), v);
	TEST(int_map_add(&im, 6, w), 5);
	TEST(int_map_find(&im, 6), w);
	TEST(int_map_count(&im), 6);
	TEST((int_map_exclude(&im, 1), EOK), EOK);
	TEST(int_map_find(&im, 1), NULL);
	TEST(int_map_find(&im, 2), u);
	TEST((int_map_exclude(&im, 7), EOK), EOK);
	TEST(int_map_find(&im, 2), u);
	TEST(int_map_find(&im, 6), w);
	TEST((int_map_exclude_index(&im, 4), EOK), EOK);
	TEST(int_map_get_index(&im, 4), NULL);
	TEST(int_map_find(&im, 3), NULL);
	TEST((int_map_destroy(&im), EOK), EOK);
	TEST(int_map_count(&im), -1);
	
	printf("\nMeasured strings test");
	
	measured_string_ref string =
	    measured_string_create_bulk("I am a measured string!", 0);
	printf("\n%x, %s at %x of %d\n", string, string->value, string->value,
	    string->length);
	
	return EOK;
}

/** @}
 */
