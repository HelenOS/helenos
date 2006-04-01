/*
 * Copyright (C) 2006 Jakub Jermar
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

void *data = (void *) 0xdeadbeef;

void test(void)
{
	btree_t t;
	int i;

	btree_create(&t);

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
	btree_insert(&t, 19, data, NULL);
	btree_insert(&t, 20, data, NULL);
	btree_insert(&t, 21, data, NULL);
	btree_insert(&t, 0, data, NULL);
	btree_insert(&t, 25, data, NULL);
	btree_insert(&t, 22, data, NULL);
	btree_insert(&t, 26, data, NULL);
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

	btree_print(&t);
	
	btree_remove(&t, 50, NULL);

}
