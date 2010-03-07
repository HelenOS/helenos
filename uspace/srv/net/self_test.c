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
 *  @{
 */

/** @file
 *  Self tests implementation.
 */

#include "configuration.h"

#if NET_SELF_TEST

#include <errno.h>
#include <malloc.h>
#include <stdio.h>

#include "include/checksum.h"
#include "structures/int_map.h"
#include "structures/char_map.h"
#include "structures/generic_char_map.h"
#include "structures/measured_strings.h"
#include "structures/dynamic_fifo.h"

#include "self_test.h"

/** Tests the function, compares the result and remembers if the result differs.
 *  @param[in] name The test name.
 *  @param[in] function_call The function to be called and checked.
 *  @param[in] result The expected result.
 */
#define TEST(name, function_call, result);	{	\
	printf("\n\t%s", (name)); 					\
	if((function_call) != (result)){			\
		printf("\tERROR\n");						\
		error = 1;									\
	}else{											\
		printf("\tOK\n");							\
	}												\
}

#if NET_SELF_TEST_INT_MAP

	INT_MAP_DECLARE(int_map, int);

	INT_MAP_IMPLEMENT(int_map, int);

#endif

#if NET_SELF_TEST_GENERIC_FIELD

	GENERIC_FIELD_DECLARE(int_field, int)

	GENERIC_FIELD_IMPLEMENT(int_field, int)

#endif

#if NET_SELF_TEST_GENERIC_CHAR_MAP

	GENERIC_CHAR_MAP_DECLARE(int_char_map, int)

	GENERIC_CHAR_MAP_IMPLEMENT(int_char_map, int)

#endif

int self_test(void){
	int error;
	int * x;
	int * y;
	int * z;
	int * u;
	int * v;
	int * w;

	error = 0;

#if NET_SELF_TEST_MEASURED_STRINGS
	measured_string_ref string;

	printf("\nMeasured strings test");
	string = measured_string_create_bulk("I am a measured string!", 0);
	printf("\n%x, %s at %x of %d", string, string->value, string->value, string->length);
	printf("\nOK");
#endif

#if NET_SELF_TEST_CHAR_MAP
	char_map_t cm;

	printf("\nChar map test");
	TEST("update ucho 3 einval", char_map_update(&cm, "ucho", 0, 3), EINVAL);
	TEST("initialize", char_map_initialize(&cm), EOK);
	TEST("exclude bla null", char_map_exclude(&cm, "bla", 0), CHAR_MAP_NULL);
	TEST("find bla null", char_map_find(&cm, "bla", 0), CHAR_MAP_NULL);
	TEST("add bla 1 eok", char_map_add(&cm, "bla", 0, 1), EOK);
	TEST("find bla 1", char_map_find(&cm, "bla", 0), 1);
	TEST("add bla 10 eexists", char_map_add(&cm, "bla", 0, 10), EEXISTS);
	TEST("update bla 2 eok", char_map_update(&cm, "bla", 0, 2), EOK);
	TEST("find bla 2", char_map_find(&cm, "bla", 0), 2);
	TEST("update ucho 2 eok", char_map_update(&cm, "ucho", 0, 2), EOK);
	TEST("exclude bla 2", char_map_exclude(&cm, "bla", 0), 2);
	TEST("exclude bla null", char_map_exclude(&cm, "bla", 0), CHAR_MAP_NULL);
	TEST("find ucho 2", char_map_find(&cm, "ucho", 0), 2);
	TEST("update ucho 3 eok", char_map_update(&cm, "ucho", 0, 3), EOK);
	TEST("find ucho 3", char_map_find(&cm, "ucho", 0), 3);
	TEST("add blabla 5 eok", char_map_add(&cm, "blabla", 0, 5), EOK);
	TEST("find blabla 5", char_map_find(&cm, "blabla", 0), 5);
	TEST("add bla 6 eok", char_map_add(&cm, "bla", 0, 6), EOK);
	TEST("find bla 6", char_map_find(&cm, "bla", 0), 6);
	TEST("exclude bla 6", char_map_exclude(&cm, "bla", 0), 6);
	TEST("find bla null", char_map_find(&cm, "bla", 0), CHAR_MAP_NULL);
	TEST("find blabla 5", char_map_find(&cm, "blabla", 0), 5);
	TEST("add auto 7 eok", char_map_add(&cm, "auto", 0, 7), EOK);
	TEST("find auto 7", char_map_find(&cm, "auto", 0), 7);
	TEST("add kara 8 eok", char_map_add(&cm, "kara", 0, 8), EOK);
	TEST("find kara 8", char_map_find(&cm, "kara", 0), 8);
	TEST("add nic 9 eok", char_map_add(&cm, "nic", 0, 9), EOK);
	TEST("find nic 9", char_map_find(&cm, "nic", 0), 9);
	TEST("find blabla 5", char_map_find(&cm, "blabla", 0), 5);
	TEST("add micnicnic 5 9 eok", char_map_add(&cm, "micnicnic", 5, 9), EOK);
	TEST("find micni 9", char_map_find(&cm, "micni", 0), 9);
	TEST("find micnicn 5 9", char_map_find(&cm, "micnicn", 5), 9);
	TEST("add 10.0.2.2 4 15 eok", char_map_add(&cm, "\x10\x0\x2\x2", 4, 15), EOK);
	TEST("find 10.0.2.2 4 15", char_map_find(&cm, "\x10\x0\x2\x2", 4), 15);
	printf("\n\tdestroy");
	char_map_destroy(&cm);
	TEST("update ucho 3 einval", char_map_update(&cm, "ucho", 0, 3), EINVAL);
	printf("\nOK");

	if(error){
		return EINVAL;
	}

#endif

#if NET_SELF_TEST_INT_MAP
	int_map_t im;

	x = (int *) malloc(sizeof(int));
	y = (int *) malloc(sizeof(int));
	z = (int *) malloc(sizeof(int));
	u = (int *) malloc(sizeof(int));
	v = (int *) malloc(sizeof(int));
	w = (int *) malloc(sizeof(int));

	im.magic = 0;
	printf("\nInt map test");
	TEST("add 1 x einval", int_map_add(&im, 1, x), EINVAL);
	TEST("count -1", int_map_count(&im), -1);
	TEST("initialize", int_map_initialize(&im), EOK);
	TEST("count 0", int_map_count(&im), 0);
	TEST("find 1 null", int_map_find(&im, 1), NULL);
	TEST("add 1 x 0", int_map_add(&im, 1, x), 0);
	TEST("find 1 x", int_map_find(&im, 1), x);
	int_map_exclude(&im, 1);
	TEST("find 1 null", int_map_find(&im, 1), NULL);
	TEST("add 1 y 1", int_map_add(&im, 1, y), 1);
	TEST("find 1 y", int_map_find(&im, 1), y);
	TEST("add 4 z 2", int_map_add(&im, 4, z), 2);
	TEST("get 2 z", int_map_get_index(&im, 2), z);
	TEST("find 4 z", int_map_find(&im, 4), z);
	TEST("find 1 y", int_map_find(&im, 1), y);
	TEST("count 3", int_map_count(&im), 3);
	TEST("add 2 u 3", int_map_add(&im, 2, u), 3);
	TEST("find 2 u", int_map_find(&im, 2), u);
	TEST("add 3 v 4", int_map_add(&im, 3, v), 4);
	TEST("find 3 v", int_map_find(&im, 3), v);
	TEST("get 4 v", int_map_get_index(&im, 4), v);
	TEST("add 6 w 5", int_map_add(&im, 6, w), 5);
	TEST("find 6 w", int_map_find(&im, 6), w);
	TEST("count 6", int_map_count(&im), 6);
	int_map_exclude(&im, 1);
	TEST("find 1 null", int_map_find(&im, 1), NULL);
	TEST("find 2 u", int_map_find(&im, 2), u);
	int_map_exclude(&im, 7);
	TEST("find 2 u", int_map_find(&im, 2), u);
	TEST("find 6 w", int_map_find(&im, 6), w);
	int_map_exclude_index(&im, 4);
	TEST("get 4 null", int_map_get_index(&im, 4), NULL);
	TEST("find 3 null", int_map_find(&im, 3), NULL);
	printf("\n\tdestroy");
	int_map_destroy(&im);
	TEST("count -1", int_map_count(&im), -1);
	printf("\nOK");

	if(error){
		return EINVAL;
	}

#endif

#if NET_SELF_TEST_GENERIC_FIELD
	int_field_t gf;

	x = (int *) malloc(sizeof(int));
	y = (int *) malloc(sizeof(int));
	z = (int *) malloc(sizeof(int));
	u = (int *) malloc(sizeof(int));
	v = (int *) malloc(sizeof(int));
	w = (int *) malloc(sizeof(int));

	gf.magic = 0;
	printf("\nGeneric field test");
	TEST("add x einval", int_field_add(&gf, x), EINVAL);
	TEST("count -1", int_field_count(&gf), -1);
	TEST("initialize", int_field_initialize(&gf), EOK);
	TEST("count 0", int_field_count(&gf), 0);
	TEST("get 1 null", int_field_get_index(&gf, 1), NULL);
	TEST("add x 0", int_field_add(&gf, x), 0);
	TEST("get 0 x", int_field_get_index(&gf, 0), x);
	int_field_exclude_index(&gf, 0);
	TEST("get 0 null", int_field_get_index(&gf, 0), NULL);
	TEST("add y 1", int_field_add(&gf, y), 1);
	TEST("get 1 y", int_field_get_index(&gf, 1), y);
	TEST("add z 2", int_field_add(&gf, z), 2);
	TEST("get 2 z", int_field_get_index(&gf, 2), z);
	TEST("get 1 y", int_field_get_index(&gf, 1), y);
	TEST("count 3", int_field_count(&gf), 3);
	TEST("add u 3", int_field_add(&gf, u), 3);
	TEST("get 3 u", int_field_get_index(&gf, 3), u);
	TEST("add v 4", int_field_add(&gf, v), 4);
	TEST("get 4 v", int_field_get_index(&gf, 4), v);
	TEST("add w 5", int_field_add(&gf, w), 5);
	TEST("get 5 w", int_field_get_index(&gf, 5), w);
	TEST("count 6", int_field_count(&gf), 6);
	int_field_exclude_index(&gf, 1);
	TEST("get 1 null", int_field_get_index(&gf, 1), NULL);
	TEST("get 3 u", int_field_get_index(&gf, 3), u);
	int_field_exclude_index(&gf, 7);
	TEST("get 3 u", int_field_get_index(&gf, 3), u);
	TEST("get 5 w", int_field_get_index(&gf, 5), w);
	int_field_exclude_index(&gf, 4);
	TEST("get 4 null", int_field_get_index(&gf, 4), NULL);
	printf("\n\tdestroy");
	int_field_destroy(&gf);
	TEST("count -1", int_field_count(&gf), -1);
	printf("\nOK");

	if(error){
		return EINVAL;
	}

#endif

#if NET_SELF_TEST_GENERIC_CHAR_MAP
	int_char_map_t icm;

	x = (int *) malloc(sizeof(int));
	y = (int *) malloc(sizeof(int));
	z = (int *) malloc(sizeof(int));
	u = (int *) malloc(sizeof(int));
	v = (int *) malloc(sizeof(int));
	w = (int *) malloc(sizeof(int));

	icm.magic = 0;
	printf("\nGeneric char map test");
	TEST("add ucho z einval", int_char_map_add(&icm, "ucho", 0, z), EINVAL);
	TEST("initialize", int_char_map_initialize(&icm), EOK);
	printf("\n\texclude bla null");
	int_char_map_exclude(&icm, "bla", 0);
	TEST("find bla null", int_char_map_find(&icm, "bla", 0), NULL);
	TEST("add bla x eok", int_char_map_add(&icm, "bla", 0, x), EOK);
	TEST("find bla x", int_char_map_find(&icm, "bla", 0), x);
	TEST("add bla y eexists", int_char_map_add(&icm, "bla", 0, y), EEXISTS);
	printf("\n\texclude bla y");
	int_char_map_exclude(&icm, "bla", 0);
	printf("\n\texclude bla null");
	int_char_map_exclude(&icm, "bla", 0);
	TEST("add blabla v eok", int_char_map_add(&icm, "blabla", 0, v), EOK);
	TEST("find blabla v", int_char_map_find(&icm, "blabla", 0), v);
	TEST("add bla w eok", int_char_map_add(&icm, "bla", 0, w), EOK);
	TEST("find bla w", int_char_map_find(&icm, "bla", 0), w);
	printf("\n\texclude bla");
	int_char_map_exclude(&icm, "bla", 0);
	TEST("find bla null", int_char_map_find(&icm, "bla", 0), NULL);
	TEST("find blabla v", int_char_map_find(&icm, "blabla", 0), v);
	TEST("add auto u eok", int_char_map_add(&icm, "auto", 0, u), EOK);
	TEST("find auto u", int_char_map_find(&icm, "auto", 0), u);
	printf("\n\tdestroy");
	int_char_map_destroy(&icm);
	TEST("add ucho z einval", int_char_map_add(&icm, "ucho", 0, z), EINVAL);
	printf("\nOK");

	if(error){
		return EINVAL;
	}

#endif

#if NET_SELF_TEST_CRC
	uint32_t value;

	printf("\nCRC computation test");
	value = ~ compute_crc32(~ 0, "123456789", 8 * 9);
	TEST("123456789", value, 0xCBF43926);
	printf("\t=> %X", value);
	value = ~ compute_crc32(~ 0, "1", 8);
	TEST("1", value, 0x83DCEFB7);
	printf("\t=> %X", value);
	value = ~ compute_crc32(~ 0, "12", 8 * 2);
	TEST("12", value, 0x4F5344CD);
	printf("\t=> %X", value);
	value = ~ compute_crc32(~ 0, "123", 8 * 3);
	TEST("123", value, 0x884863D2);
	printf("\t=> %X", value);
	value = ~ compute_crc32(~ 0, "1234", 8 * 4);
	TEST("1234", value, 0x9BE3E0A3);
	printf("\t=> %X", value);
	value = ~ compute_crc32(~ 0, "12345678", 8 * 8);
	TEST("12345678", value, 0x9AE0DAAF);
	printf("\t=> %X", value);
	value = ~ compute_crc32(~ 0, "ahoj pane", 8 * 9);
	TEST("ahoj pane", value, 0x5FC3D706);
	printf("\t=> %X", value);

	if(error){
		return EINVAL;
	}

#endif

#if NET_SELF_TEST_DYNAMIC_FIFO
	dyn_fifo_t fifo;

	printf("\nDynamic fifo test");
	TEST("add 1 einval", dyn_fifo_push(&fifo, 1, 0), EINVAL);
	TEST("initialize", dyn_fifo_initialize(&fifo, 1), EOK);
	TEST("add 1 eok", dyn_fifo_push(&fifo, 1, 0), EOK);
	TEST("pop 1", dyn_fifo_pop(&fifo), 1);
	TEST("pop enoent", dyn_fifo_pop(&fifo), ENOENT);
	TEST("add 2 eok", dyn_fifo_push(&fifo, 2, 1), EOK);
	TEST("add 3 enomem", dyn_fifo_push(&fifo, 3, 1), ENOMEM);
	TEST("add 3 eok", dyn_fifo_push(&fifo, 3, 0), EOK);
	TEST("pop 2", dyn_fifo_pop(&fifo), 2);
	TEST("pop 3", dyn_fifo_pop(&fifo), 3);
	TEST("add 4 eok", dyn_fifo_push(&fifo, 4, 2), EOK);
	TEST("add 5 eok", dyn_fifo_push(&fifo, 5, 2), EOK);
	TEST("add 6 enomem", dyn_fifo_push(&fifo, 6, 2), ENOMEM);
	TEST("add 6 eok", dyn_fifo_push(&fifo, 6, 5), EOK);
	TEST("add 7 eok", dyn_fifo_push(&fifo, 7, 5), EOK);
	TEST("pop 4", dyn_fifo_pop(&fifo), 4);
	TEST("pop 5", dyn_fifo_pop(&fifo), 5);
	TEST("add 8 eok", dyn_fifo_push(&fifo, 8, 5), EOK);
	TEST("add 9 eok", dyn_fifo_push(&fifo, 9, 5), EOK);
	TEST("add 10 eok", dyn_fifo_push(&fifo, 10, 6), EOK);
	TEST("add 11 eok", dyn_fifo_push(&fifo, 11, 6), EOK);
	TEST("pop 6", dyn_fifo_pop(&fifo), 6);
	TEST("pop 7", dyn_fifo_pop(&fifo), 7);
	TEST("add 12 eok", dyn_fifo_push(&fifo, 12, 6), EOK);
	TEST("add 13 eok", dyn_fifo_push(&fifo, 13, 6), EOK);
	TEST("add 14 enomem", dyn_fifo_push(&fifo, 14, 6), ENOMEM);
	TEST("add 14 eok", dyn_fifo_push(&fifo, 14, 8), EOK);
	TEST("pop 8", dyn_fifo_pop(&fifo), 8);
	TEST("pop 9", dyn_fifo_pop(&fifo), 9);
	TEST("pop 10", dyn_fifo_pop(&fifo), 10);
	TEST("pop 11", dyn_fifo_pop(&fifo), 11);
	TEST("pop 12", dyn_fifo_pop(&fifo), 12);
	TEST("pop 13", dyn_fifo_pop(&fifo), 13);
	TEST("pop 14", dyn_fifo_pop(&fifo), 14);
	TEST("destroy", dyn_fifo_destroy(&fifo), EOK);
	TEST("add 15 einval", dyn_fifo_push(&fifo, 1, 0), EINVAL);
	if(error){
		return EINVAL;
	}

#endif

	return EOK;
}

#endif

/** @}
 */
