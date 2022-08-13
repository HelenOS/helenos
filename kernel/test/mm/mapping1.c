/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <test.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <arch/mm/page.h>
#include <mm/km.h>
#include <typedefs.h>
#include <arch.h>

#define TEST_MAGIC  UINT32_C(0x01234567)

const char *test_mapping1(void)
{
	uintptr_t frame = frame_alloc(1, FRAME_HIGHMEM, 0);

	uintptr_t page0 = km_map(frame, FRAME_SIZE, FRAME_SIZE,
	    PAGE_READ | PAGE_WRITE | PAGE_CACHEABLE);
	TPRINTF("Virtual address %p mapped to physical address %p.\n",
	    (void *) page0, (void *) frame);

	uintptr_t page1 = km_map(frame, FRAME_SIZE, FRAME_SIZE,
	    PAGE_READ | PAGE_WRITE | PAGE_CACHEABLE);
	TPRINTF("Virtual address %p mapped to physical address %p.\n",
	    (void *) page1, (void *) frame);

	for (unsigned int i = 0; i < 2; i++) {
		TPRINTF("Writing magic using the first virtual address.\n");

		*((uint32_t *) page0) = TEST_MAGIC;

		TPRINTF("Reading magic using the second virtual address.\n");

		uint32_t v = *((uint32_t *) page1);

		if (v != TEST_MAGIC) {
			km_unmap(page0, PAGE_SIZE);
			km_unmap(page1, PAGE_SIZE);
			frame_free(frame, 1);
			return "Criss-cross read does not match the value written.";
		}

		TPRINTF("Writing zero using the second virtual address.\n");

		*((uint32_t *) page1) = 0;

		TPRINTF("Reading zero using the first virtual address.\n");

		v = *((uint32_t *) page0);

		if (v != 0) {
			km_unmap(page0, PAGE_SIZE);
			km_unmap(page1, PAGE_SIZE);
			frame_free(frame, 1);
			return "Criss-cross read does not match the value written.";
		}
	}

	km_unmap(page0, PAGE_SIZE);
	km_unmap(page1, PAGE_SIZE);
	frame_free(frame, 1);

	return NULL;
}
