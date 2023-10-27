/*
 * Copyright (c) 2007 Michal Kebrt
 * Copyright (c) 2018 Jiří Zárevúcky
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

#include <payload.h>

#include <align.h>
#include <printf.h>
#include <arch/arch.h>
#include <tar.h>
#include <gzip.h>
#include <stdbool.h>
#include <mem.h>
#include <errno.h>
#include <str.h>
#include <halt.h>

static const char *ext(const char *s)
{
	const char *last = s;

	while (*s) {
		if (*s == '.')
			last = s;

		s++;
	}

	if (*last == '.')
		return last;

	return NULL;
}

static void basename(char *s)
{
	char *e = (char *) ext(s);
	if ((e != NULL) && (str_cmp(e, ".gz") == 0))
		*e = '\0';
}

static bool overlaps(uint8_t *start1, uint8_t *end1,
    uint8_t *start2, uint8_t *end2)
{
	return !(end1 <= start2 || end2 <= start1);
}

static bool extract_component(uint8_t **cstart, uint8_t *cend,
    uint8_t *ustart, uint8_t *uend, uintptr_t actual_ustart,
    void (*clear_cache)(void *, size_t), task_t *task)
{
	const char *name;
	size_t packed_size;

	if (!tar_info(*cstart, cend, &name, &packed_size))
		return false;

	const uint8_t *data = *cstart + TAR_BLOCK_SIZE;
	*cstart += TAR_BLOCK_SIZE + ALIGN_UP(packed_size, TAR_BLOCK_SIZE);

	bool gz = gzip_check(data, packed_size);
	size_t unpacked_size = gz ? gzip_size(data, packed_size) : packed_size;

	/* Components must be page-aligned. */
	uint8_t *new_ustart = (uint8_t *) ALIGN_UP((uintptr_t) ustart, PAGE_SIZE);
	actual_ustart += new_ustart - ustart;
	ustart = new_ustart;
	uint8_t *comp_end = ustart + unpacked_size;

	/* Check limits and overlap. */
	if (overlaps(ustart, comp_end, loader_start, loader_end)) {
		/* Move the component after bootloader. */
		printf("%s would overlap bootloader, moving to %p.\n", name, loader_end);
		uint8_t *new_ustart = (uint8_t *) ALIGN_UP((uintptr_t) loader_end, PAGE_SIZE);
		actual_ustart += new_ustart - ustart;
		ustart = new_ustart;
		comp_end = ustart + unpacked_size;
	}

	if (comp_end > uend) {
		printf("Not enough available memory for remaining components"
		    " (at least %zd more required).\n", comp_end - uend);
		halt();
	}

	printf(" %p|%p: %s image (%zu/%zu bytes)\n", (void *) actual_ustart,
	    ustart, name, unpacked_size, packed_size);

	if (task) {
		task->addr = (void *) actual_ustart;
		task->size = unpacked_size;
		str_cpy(task->name, BOOTINFO_TASK_NAME_BUFLEN, name);
		/* Remove .gz extension */
		if (gz)
			basename(task->name);
	}

	if (gz) {
		int rc = gzip_expand(data, packed_size, ustart, unpacked_size);
		if (rc != EOK) {
			printf("\n%s: Inflating error %d\n", name, rc);
			halt();
		}
	} else {
		memcpy(ustart, data, unpacked_size);
	}

	if (clear_cache)
		clear_cache(ustart, unpacked_size);

	return true;
}

/* @return Bytes needed for unpacked payload. */
size_t payload_unpacked_size(void)
{
	size_t sz = 0;
	uint8_t *start = payload_start;
	const char *name;
	size_t packed_size;

	while (tar_info(start, payload_end, &name, &packed_size)) {
		sz = ALIGN_UP(sz, PAGE_SIZE);
		if (gzip_check(start + TAR_BLOCK_SIZE, packed_size))
			sz += gzip_size(start + TAR_BLOCK_SIZE, packed_size);
		else
			sz += packed_size;

		start += TAR_BLOCK_SIZE + ALIGN_UP(packed_size, TAR_BLOCK_SIZE);
	}

	return sz;
}

/**
 * Extract the payload (kernel, loader, init binaries and the initrd image).
 *
 * @param bootinfo      Pointer to the structure where the actual placement
 *                      of components is recorded.
 *
 * @param kernel_dest   Address of the kernel in the bootloader's address space.
 *                      Kernel is the only part of the payload that has a fixed
 *                      location and cannot be moved. If the kernel doesn't fit
 *                      or would overlap bootloader, bootloader halts.
 *
 * @param mem_end       End of usable contiguous memory.
 *                      The caller guarantees that the entire area between
 *                      kernel_start and mem_end is free and safe to write to,
 *                      save possibly for the interval [loader_start, loader_end).
 *                      All components are placed in this area. If there is not
 *                      enough space for all components, bootloader halts.
 *
 * @param kernel_start  Address the kernel will have in the kernel's own
 *                      address space.
 *
 * @param clear_cache   Caller-provided function for assuring cache coherence,
 *                      whatever that means for a given platform. May be NULL.
 */
void extract_payload(taskmap_t *tmap, uint8_t *kernel_dest, uint8_t *mem_end,
    uintptr_t kernel_start, void (*clear_cache)(void *, size_t))
{
	task_t task;
	memset(&task, 0, sizeof(task));

	printf("Boot loader: %p -> %p\n", loader_start, loader_end);
	printf("Payload: %p -> %p\n", payload_start, payload_end);
	printf("Kernel load address: %p\n", kernel_dest);
	printf("Kernel start: %p\n", (void *) kernel_start);
	printf("RAM end: %p (%zd bytes available)\n", mem_end,
	    mem_end - kernel_dest);

	size_t payload_size = payload_end - payload_start;
	uint8_t *real_payload_start;
	uint8_t *real_payload_end;

	if (overlaps(kernel_dest, mem_end, payload_start, payload_end)) {
		/*
		 * First, move the payload to the very end of available memory,
		 * to make space for the unpacked data.
		 */
		real_payload_start = (uint8_t *) ALIGN_DOWN((uintptr_t)(mem_end - payload_size), PAGE_SIZE);
		real_payload_end = real_payload_start + payload_size;
		memmove(real_payload_start, payload_start, payload_size);

		printf("Moved payload: %p -> %p\n", real_payload_start, real_payload_end);
	} else {
		real_payload_start = payload_start;
		real_payload_end = payload_end;
	}

	printf("\nInflating components ... \n");

	uint8_t *end = mem_end;

	if (real_payload_end > kernel_dest && real_payload_start < mem_end)
		end = real_payload_start;

	/* Kernel is always first. */
	if (!extract_component(&real_payload_start, real_payload_end,
	    kernel_dest, end, kernel_start, clear_cache, &task)) {
		printf("There is no kernel.\n");
		halt();
	}

	if ((uintptr_t) task.addr != kernel_start) {
		printf("Couldn't load kernel at the requested address.\n");
		halt();
	}

	tmap->cnt = 0;

	for (int i = 0; i <= TASKMAP_MAX_RECORDS; i++) {
		/*
		 * `task` holds the location and size of the previous component.
		 */
		uintptr_t actual_dest =
		    ALIGN_UP((uintptr_t) task.addr + task.size, PAGE_SIZE);
		uint8_t *dest = kernel_dest + (actual_dest - kernel_start);

		if (real_payload_end > dest && real_payload_start < mem_end)
			end = real_payload_start;

		if (!extract_component(&real_payload_start, real_payload_end,
		    dest, end, actual_dest, clear_cache, &task))
			break;

		if (i >= TASKMAP_MAX_RECORDS) {
			printf("More components than the maximum of %d.\n",
			    TASKMAP_MAX_RECORDS);
			halt();
		}

		tmap->tasks[i] = task;
		tmap->cnt = i + 1;
	}

	printf("Done.\n");
}
