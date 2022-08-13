/*
 * SPDX-FileCopyrightText: 2006 Sergey Bondari
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */

/**
 * @file
 * @brief Kernel ELF loader.
 */

#include <assert.h>
#include <lib/elf.h>
#include <typedefs.h>
#include <mm/as.h>
#include <mm/frame.h>
#include <mm/slab.h>
#include <align.h>
#include <macros.h>
#include <arch.h>
#include <str.h>

#include <lib/elf_load.h>

static errno_t load_segment(elf_segment_header_t *, elf_header_t *, as_t *);

/** ELF loader
 *
 * @param header Pointer to ELF header in memory
 * @param as     Created and properly mapped address space
 * @param flags  A combination of ELD_F_*
 *
 * @return EOK on success
 *
 */
errno_t elf_load(elf_header_t *header, as_t *as)
{
	/* Identify ELF */
	if ((header->e_ident[EI_MAG0] != ELFMAG0) ||
	    (header->e_ident[EI_MAG1] != ELFMAG1) ||
	    (header->e_ident[EI_MAG2] != ELFMAG2) ||
	    (header->e_ident[EI_MAG3] != ELFMAG3))
		return EINVAL;

	/* Identify ELF compatibility */
	if ((header->e_ident[EI_DATA] != ELF_DATA_ENCODING) ||
	    (header->e_machine != ELF_MACHINE) ||
	    (header->e_ident[EI_VERSION] != EV_CURRENT) ||
	    (header->e_version != EV_CURRENT) ||
	    (header->e_ident[EI_CLASS] != ELF_CLASS))
		return EINVAL;

	if (header->e_phentsize != sizeof(elf_segment_header_t))
		return EINVAL;

	/* Check if the object type is supported. */
	if (header->e_type != ET_EXEC)
		return ENOTSUP;

	/* Check if the ELF image starts on a page boundary */
	if (ALIGN_UP((uintptr_t) header, PAGE_SIZE) != (uintptr_t) header)
		return ENOTSUP;

	/* Walk through all segment headers and process them. */
	elf_half i;
	for (i = 0; i < header->e_phnum; i++) {
		elf_segment_header_t *seghdr =
		    &((elf_segment_header_t *)(((uint8_t *) header) +
		    header->e_phoff))[i];

		if (seghdr->p_type != PT_LOAD)
			continue;

		errno_t rc = load_segment(seghdr, header, as);
		if (rc != EOK)
			return rc;
	}

	return EOK;
}

/** Load segment described by program header entry.
 *
 * @param entry Program header entry describing segment to be loaded.
 * @param elf   ELF header.
 * @param as    Address space into wich the ELF is being loaded.
 *
 * @return EOK on success, error code otherwise.
 *
 */
errno_t load_segment(elf_segment_header_t *entry, elf_header_t *elf, as_t *as)
{
	mem_backend_data_t backend_data;

	if (entry->p_align > 1) {
		if ((entry->p_offset % entry->p_align) !=
		    (entry->p_vaddr % entry->p_align))
			return EINVAL;
	}

	unsigned int flags = 0;

	if (entry->p_flags & PF_X)
		flags |= AS_AREA_EXEC;

	if (entry->p_flags & PF_W)
		flags |= AS_AREA_WRITE;

	if (entry->p_flags & PF_R)
		flags |= AS_AREA_READ;

	flags |= AS_AREA_CACHEABLE;

	/*
	 * Align vaddr down, inserting a little "gap" at the beginning.
	 * Adjust area size, so that its end remains in place.
	 *
	 */
	uintptr_t base = ALIGN_DOWN(entry->p_vaddr, PAGE_SIZE);
	size_t mem_sz = entry->p_memsz + (entry->p_vaddr - base);

	backend_data.elf_base = base;
	backend_data.elf = elf;
	backend_data.segment = entry;

	as_area_t *area = as_area_create(as, flags, mem_sz,
	    AS_AREA_ATTR_NONE, &elf_backend, &backend_data, &base, 0);
	if (!area)
		return ENOMEM;

	/*
	 * The segment will be mapped on demand by elf_page_fault().
	 *
	 */

	return EOK;
}

/** @}
 */
