/*
 * Copyright (c) 2011 Vojtech Horky
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

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <str_error.h>
#include <as.h>
#include <errno.h>
#include "../tester.h"

#define BUFFER1_PAGES  4
#define BUFFER2_PAGES  2

static void *create_as_area(size_t size)
{
	TPRINTF("Creating AS area...\n");
	
	void *result = as_area_create(AS_AREA_ANY, size,
	    AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE, AS_AREA_UNPAGED);
	if (result == AS_MAP_FAILED)
		return NULL;
	
	return result;
}

static void touch_area(void *area, size_t size)
{
	TPRINTF("Touching (faulting-in) AS area...\n");
	
	char *ptr = (char *)area;
	
	while (size > 0) {
		*ptr = 0;
		size--;
		ptr++;
	}
}

#define VERIFY_MAPPING(area, page_count, expected_rc) \
    verify_mapping((area), (page_count), (expected_rc), #expected_rc)

static bool verify_mapping(void *area, int page_count, errno_t expected_rc,
    const char *expected_rc_str)
{
	TPRINTF("Verifying mapping (expected: %s).\n", expected_rc_str);
	int i;
	for (i = 0; i < page_count; i++) {
		void *page_start = ((char *) area) + PAGE_SIZE * i;
		errno_t rc = as_get_physical_mapping(page_start, NULL);
		if (rc != expected_rc) {
			TPRINTF("as_get_physical_mapping() = %s != %s\n",
			    str_error_name(rc), str_error_name(expected_rc));
			return false;
		}
	}
	return true;
}

const char *test_mapping1(void)
{
	errno_t rc;
	
	size_t buffer1_len = BUFFER1_PAGES * PAGE_SIZE;
	size_t buffer2_len = BUFFER2_PAGES * PAGE_SIZE;
	void *buffer1 = create_as_area(buffer1_len);
	void *buffer2 = create_as_area(buffer2_len);
	if (!buffer1 || !buffer2) {
		return "Cannot allocate memory";
	}
	
	touch_area(buffer1, buffer1_len);
	touch_area(buffer2, buffer2_len);
	
	/* Now verify that mapping to physical frames exist. */
	if (!VERIFY_MAPPING(buffer1, BUFFER1_PAGES, EOK)) {
		return "Failed to find mapping (buffer1)";
	}
	if (!VERIFY_MAPPING(buffer2, BUFFER2_PAGES, EOK)) {
		return "Failed to find mapping (buffer2)";
	}
	
	/* Let's destroy the buffer1 area and access it again. */
	rc = as_area_destroy(buffer1);
	if (rc != EOK) {
		return "Failed to destroy AS area";
	}
	if (!VERIFY_MAPPING(buffer1, BUFFER1_PAGES, ENOENT)) {
		return "Mapping of destroyed area still exists";
	}
	
	/* clean-up */
	rc = as_area_destroy(buffer2);
	if (rc != EOK) {
		return "Failed to destroy AS area";
	}
	
	return NULL;
}
