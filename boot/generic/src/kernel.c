/*
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

#include <abi/elf.h>
#include <halt.h>
#include <printf.h>
#include <kernel.h>
#include <stdbool.h>

// FIXME: elf_is_valid is a duplicate of the same-named libc function.

// TODO: better kernel ELF loading
//
// Currently the boot loader is very primitive. It loads the ELF file as
// a contiguous span starting at a predefined offset, and then checks load
// segments in it to make sure they are correctly positioned. Ideally, this
// should change to a more flexible loader that actually loads based on the
// kernel's ELF segments. There would still be some restrictions however.
// ELF vaddr and paddr fields offer some flexibility in their interpretation,
// so I propose the following scheme, to correctly express everything various
// architectures require:
//
//     - in vaddr and paddr fields, addresses numerically in the lower half are
//       interpreted as physical addresses, addresses in the upper half are
//       interpreted as virtual addresses.
//
//     - If vaddr is a virtual address, the segment is mapped into the kernel's
//       virtual address space at vaddr.
//
//     - If vaddr is a physical address, it must be the same as paddr.
//       Loader loads the segment at the given physical address, but does not
//       map it into the kernel's virtual address space. Symbols defined in this
//       segment are only accessible with paging disabled.
//
//     - If paddr is a physical address, the loader must load the segment at
//       physical address paddr, or die trying.
//
//     - If paddr is a virtual address, it must be the same as vaddr.
//       Loader may allocate the physical location arbitrarily.
//
//     - If the kernel is a Position Independent Executable, all this is
//       irrelevant, paddr must be the same as vaddr, vaddr is always the
//       virtual address offset, and loader can choose the virtual address
//       base arbitrarily within some predefined constraints. We might want
//       to support PIE kernel on architectures that need some code at fixed
//       physical address. In that case, the "real mode" code should probably
//       be in a separate ELF file from the rest of kernel.
//

/**
 * Checks that the ELF header is valid for the running system.
 */
static bool elf_is_valid(const elf_header_t *header)
{
	// TODO: check more things
	// TODO: debug output

	if (header->e_ident[EI_MAG0] != ELFMAG0 ||
	    header->e_ident[EI_MAG1] != ELFMAG1 ||
	    header->e_ident[EI_MAG2] != ELFMAG2 ||
	    header->e_ident[EI_MAG3] != ELFMAG3) {
		return false;
	}

	if (header->e_ident[EI_DATA] != ELF_DATA_ENCODING ||
	    header->e_machine != ELF_MACHINE ||
	    header->e_ident[EI_VERSION] != EV_CURRENT ||
	    header->e_version != EV_CURRENT ||
	    header->e_ident[EI_CLASS] != ELF_CLASS) {
		return false;
	}

	if (header->e_phentsize != sizeof(elf_segment_header_t)) {
		return false;
	}

	if (header->e_type != ET_EXEC && header->e_type != ET_DYN) {
		return false;
	}

	if (header->e_phoff == 0) {
		return false;
	}

	return true;
}

uintptr_t check_kernel(void *start)
{
	return check_kernel_translated(start, (uintptr_t) start);
}

/**
 * Checks that the kernel ELF image is valid, and returns the entry point
 * address. We check that the image's load addresses match the actual location.
 *
 * @param start  Pointer to the start of the ELF file in memory.
 * @param actual_addr  Start address where the kernel is moved after the check
 *                     but before it is executed.
 *
 * @return  Entry point address in *kernel's* address space.
 */
uintptr_t check_kernel_translated(void *start, uintptr_t actual_addr)
{
	elf_header_t *header = (elf_header_t *) start;

	if (!elf_is_valid(header)) {
		printf("Kernel is not a valid ELF image.\n");
		halt();
	}

	elf_segment_header_t *phdr =
	    (elf_segment_header_t *) ((uintptr_t) start + header->e_phoff);

	/* Walk through PT_LOAD headers, to find out the size of the module. */
	for (int i = 0; i < header->e_phnum; i++) {
		if (phdr[i].p_type != PT_LOAD)
			continue;

		uintptr_t expected = actual_addr + phdr[i].p_offset;
		uintptr_t got = phdr[i].p_paddr;
		if (expected != got) {
			printf("Incorrect kernel load address. "
			    "Expected: %p, got %p\n",
			    (void *) expected, (void *) got);
			halt();
		}

		if (phdr[i].p_filesz != phdr[i].p_memsz) {
			printf("Kernel's memory size is greater than its file"
			    " size. We don't currently support that.\n");
			halt();
		}
	}

	return (uintptr_t) header->e_entry;
}
