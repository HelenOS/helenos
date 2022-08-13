/*
 * SPDX-FileCopyrightText: 2006 Sergey Bondari
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 * @brief ELF loader structures and public functions.
 */

#ifndef ELF_MOD_H_
#define ELF_MOD_H_

#include <elf/elf.h>
#include <stddef.h>
#include <stdint.h>
#include <loader/pcb.h>

typedef enum {
	/** Leave all segments in RW access mode. */
	ELDF_RW = 1
} eld_flags_t;

/** TLS info for a module */
typedef struct {
	/** tdata section image */
	void *tdata;
	/** Size of tdata section image in bytes */
	size_t tdata_size;
	/** Size of tbss section */
	size_t tbss_size;
	/** Alignment of TLS initialization image */
	size_t tls_align;
} elf_tls_info_t;

/**
 * Some data extracted from the headers are stored here
 */
typedef struct {
	/** Entry point */
	entry_point_t entry;

	/** The base address where the file has been loaded.
	 *  Points to the ELF file header.
	 */
	void *base;

	/** ELF interpreter name or NULL if statically-linked */
	const char *interp;

	/** Pointer to the dynamic section */
	void *dynamic;

	/** TLS info */
	elf_tls_info_t tls;
} elf_finfo_t;

/**
 * Holds information about an ELF binary being loaded.
 */
typedef struct {
	/** Filedescriptor of the file from which we are loading */
	int fd;

	/** Difference between run-time addresses and link-time addresses */
	uintptr_t bias;

	/** Flags passed to the ELF loader. */
	eld_flags_t flags;

	/** Store extracted info here */
	elf_finfo_t *info;
} elf_ld_t;

extern errno_t elf_load_file(int, eld_flags_t, elf_finfo_t *);
extern errno_t elf_load_file_name(const char *, eld_flags_t, elf_finfo_t *);

#endif

/** @}
 */
