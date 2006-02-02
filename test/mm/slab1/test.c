/*
 * Copyright (C) 2006 Ondrej Palkovsky
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
#include <mm/slab.h>
#include <print.h>

#define VAL_SIZE    128
#define VAL_COUNT   1024

void * data[16384];

void test(void) 
{
	slab_cache_t *cache;
	int i;
	

	printf("Creating cache.\n");
	cache = slab_cache_create("test_cache", VAL_SIZE, 0, NULL, NULL, SLAB_CACHE_NOMAGAZINE);
	printf("Destroying cache.\n");
	slab_cache_destroy(cache);

	printf("Creating cache.\n");
	cache = slab_cache_create("test_cache", VAL_SIZE, 0, NULL, NULL, 
				  SLAB_CACHE_NOMAGAZINE);
	
	printf("Allocating %d items...", VAL_COUNT);
	for (i=0; i < VAL_COUNT; i++) {
		data[i] = slab_alloc(cache, 0);
	}
	printf("done.\n");
	printf("Freeing %d items...", VAL_COUNT);
	for (i=0; i < VAL_COUNT; i++) {
		slab_free(cache, data[i]);
	}
	printf("done.\n");

	printf("Allocating %d items...", VAL_COUNT);
	for (i=0; i < VAL_COUNT; i++) {
		data[i] = slab_alloc(cache, 0);
	}
	printf("done.\n");

	slab_print_list();
	printf("Freeing %d items...", VAL_COUNT/2);
	for (i=VAL_COUNT-1; i >= VAL_COUNT/2; i--) {
		slab_free(cache, data[i]);
	}
	printf("done.\n");	

	printf("Allocating %d items...", VAL_COUNT/2);
	for (i=VAL_COUNT/2; i < VAL_COUNT; i++) {
		data[i] = slab_alloc(cache, 0);
	}
	printf("done.\n");
	printf("Freeing %d items...", VAL_COUNT);
	for (i=0; i < VAL_COUNT; i++) {
		slab_free(cache, data[i]);
	}
	printf("done.\n");	
	slab_print_list();
	slab_cache_destroy(cache);

	printf("Test complete.\n");
}
