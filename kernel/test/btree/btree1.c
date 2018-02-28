/*
 * Copyright (c) 2006 Jakub Jermar
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

#include <test.h>
#include <print.h>
#include <adt/btree.h>
#include <debug.h>

static void *data = (void *) 0xdeadbeef;

const char *test_btree1(void)
{
	btree_t t;
	int i;

	btree_create(&t);

	TPRINTF("Inserting keys.\n");
	btree_insert(&t, 19, data, NULL);
	btree_insert(&t, 20, data, NULL);
	btree_insert(&t, 21, data, NULL);
	btree_insert(&t, 0, data, NULL);
	btree_insert(&t, 25, data, NULL);
	btree_insert(&t, 22, data, NULL);
	btree_insert(&t, 26, data, NULL);
	btree_insert(&t, 23, data, NULL);
	btree_insert(&t, 24, data, NULL);
	btree_insert(&t, 5, data, NULL);
	btree_insert(&t, 1, data, NULL);
	btree_insert(&t, 4, data, NULL);
	btree_insert(&t, 28, data, NULL);
	btree_insert(&t, 29, data, NULL);
	btree_insert(&t, 7, data, NULL);
	btree_insert(&t, 8, data, NULL);
	btree_insert(&t, 9, data, NULL);
	btree_insert(&t, 17, data, NULL);
	btree_insert(&t, 18, data, NULL);
	btree_insert(&t, 2, data, NULL);
	btree_insert(&t, 3, data, NULL);
	btree_insert(&t, 6, data, NULL);
	btree_insert(&t, 10, data, NULL);
	btree_insert(&t, 11, data, NULL);
	btree_insert(&t, 12, data, NULL);
	btree_insert(&t, 13, data, NULL);
	btree_insert(&t, 14, data, NULL);
	btree_insert(&t, 15, data, NULL);
	btree_insert(&t, 16, data, NULL);
	btree_insert(&t, 27, data, NULL);

	for (i = 30; i < 50; i++)
		btree_insert(&t, i, data, NULL);
	for (i = 100; i >= 50; i--)
		btree_insert(&t, i, data, NULL);

	if (!test_quiet)
		btree_print(&t);

	TPRINTF("Removing keys.\n");
	btree_remove(&t, 50, NULL);
	btree_remove(&t, 49, NULL);
	btree_remove(&t, 51, NULL);
	btree_remove(&t, 46, NULL);
	btree_remove(&t, 45, NULL);
	btree_remove(&t, 48, NULL);
	btree_remove(&t, 53, NULL);
	btree_remove(&t, 47, NULL);
	btree_remove(&t, 52, NULL);
	btree_remove(&t, 54, NULL);
	btree_remove(&t, 65, NULL);
	btree_remove(&t, 60, NULL);
	btree_remove(&t, 99, NULL);
	btree_remove(&t, 97, NULL);
	btree_remove(&t, 57, NULL);
	btree_remove(&t, 58, NULL);
	btree_remove(&t, 61, NULL);
	btree_remove(&t, 64, NULL);
	btree_remove(&t, 56, NULL);
	btree_remove(&t, 41, NULL);

	for (i = 5; i < 20; i++)
		btree_remove(&t, i, NULL);

	btree_remove(&t, 2, NULL);
	btree_remove(&t, 43, NULL);
	btree_remove(&t, 22, NULL);
	btree_remove(&t, 100, NULL);
	btree_remove(&t, 98, NULL);
	btree_remove(&t, 96, NULL);
	btree_remove(&t, 66, NULL);
	btree_remove(&t, 1, NULL);

	for (i = 70; i < 90; i++)
		btree_remove(&t, i, NULL);

	btree_remove(&t, 20, NULL);
	btree_remove(&t, 0, NULL);
	btree_remove(&t, 40, NULL);
	btree_remove(&t, 3, NULL);
	btree_remove(&t, 4, NULL);
	btree_remove(&t, 21, NULL);
	btree_remove(&t, 44, NULL);
	btree_remove(&t, 55, NULL);
	btree_remove(&t, 62, NULL);
	btree_remove(&t, 26, NULL);
	btree_remove(&t, 27, NULL);
	btree_remove(&t, 28, NULL);
	btree_remove(&t, 29, NULL);
	btree_remove(&t, 30, NULL);
	btree_remove(&t, 31, NULL);
	btree_remove(&t, 32, NULL);
	btree_remove(&t, 33, NULL);
	btree_remove(&t, 93, NULL);
	btree_remove(&t, 95, NULL);
	btree_remove(&t, 94, NULL);
	btree_remove(&t, 69, NULL);
	btree_remove(&t, 68, NULL);
	btree_remove(&t, 92, NULL);
	btree_remove(&t, 91, NULL);
	btree_remove(&t, 67, NULL);
	btree_remove(&t, 63, NULL);
	btree_remove(&t, 90, NULL);
	btree_remove(&t, 59, NULL);
	btree_remove(&t, 23, NULL);
	btree_remove(&t, 24, NULL);
	btree_remove(&t, 25, NULL);
	btree_remove(&t, 37, NULL);
	btree_remove(&t, 38, NULL);
	btree_remove(&t, 42, NULL);
	btree_remove(&t, 39, NULL);
	btree_remove(&t, 34, NULL);
	btree_remove(&t, 35, NULL);
	btree_remove(&t, 36, NULL);

	if (!test_quiet)
		btree_print(&t);

	return NULL;
}
