/*
 * SPDX-FileCopyrightText: 2013 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup drvusbehci
 * @{
 */
/** @file
 * @brief EHCI driver
 */
#ifndef DRV_EHCI_UTILS_MALLOC32_H
#define DRV_EHCI_UTILS_MALLOC32_H

#include <align.h>
#include <as.h>
#include <ddi.h>
#include <errno.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Generic TDs and EDs require 16byte alignment,
 * Isochronous TD require 32byte alignment,
 * buffers do not have to be aligned.
 */
#define EHCI_ALIGN   32

#define EHCI_REQUIRED_PAGE_SIZE   4096

/** Get physical address translation
 *
 * @param[in] addr Virtual address to translate
 * @return Physical address if exists, NULL otherwise.
 */
static inline uintptr_t addr_to_phys(const void *addr)
{
	uintptr_t result;
	errno_t ret = as_get_physical_mapping(addr, &result);

	if (ret != EOK)
		return 0;

	return result;
}

/** Physical mallocator simulator
 *
 * @param[in] size Size of the required memory space
 * @return Address of the aligned and big enough memory place, NULL on failure.
 */
static inline void *malloc32(size_t size)
{
	uintptr_t phys;
	void *address = AS_AREA_ANY;
	size_t real_size = ALIGN_UP(size, PAGE_SIZE);

	const errno_t ret = dmamem_map_anonymous(real_size,
	    DMAMEM_4GiB, AS_AREA_READ | AS_AREA_WRITE, 0, &phys,
	    &address);

	if (ret == EOK) {
		/*
		 * Poison, accessing it should be enough to make sure
		 * the location is mapped, but poison works better
		 */
		memset(address, 0x5, real_size);
		return address;
	}
	return NULL;
}

/** Physical mallocator simulator
 *
 * @param[in] addr Address of the place allocated by malloc32
 */
static inline void free32(void *addr)
{
	dmamem_unmap_anonymous(addr);
}

/** Create 4KB page mapping
 *
 * @return Address of the mapped page, NULL on failure.
 */
static inline void *get_page()
{
	return malloc32(PAGE_SIZE);
}

static inline void return_page(void *page)
{
	free32(page);
}

#endif
/**
 * @}
 */
