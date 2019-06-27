/*
 * Copyright (c) 2006 Sergey Bondari
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

/** @addtogroup abi_generic
 * @{
 */
/** @file
 */

#ifndef _ABI_ELF_H_
#define _ABI_ELF_H_

#include <stdint.h>
#include <abi/arch/elf.h>

/**
 * Current ELF version
 */
enum {
	EV_CURRENT = 1,
};

/**
 * ELF types
 */
enum elf_type {
	ET_NONE   = 0,       /* No type */
	ET_REL    = 1,       /* Relocatable file */
	ET_EXEC   = 2,       /* Executable */
	ET_DYN    = 3,       /* Shared object */
	ET_CORE   = 4,       /* Core */
};

enum {
	ET_LOPROC = 0xff00,  /* Lowest processor specific */
	ET_HIPROC = 0xffff,  /* Highest processor specific */
};

/**
 * ELF machine types
 */
enum elf_machine {
	EM_NO          = 0,    /* No machine */
	EM_SPARC       = 2,    /* SPARC */
	EM_386         = 3,    /* i386 */
	EM_MIPS        = 8,    /* MIPS RS3000 */
	EM_MIPS_RS3_LE = 10,   /* MIPS RS3000 LE */
	EM_PPC         = 20,   /* PPC32 */
	EM_PPC64       = 21,   /* PPC64 */
	EM_ARM         = 40,   /* ARM */
	EM_SPARCV9     = 43,   /* SPARC64 */
	EM_IA_64       = 50,   /* IA-64 */
	EM_X86_64      = 62,   /* AMD64/EMT64 */
	EM_AARCH64     = 183,  /* ARM 64-bit architecture */
	EM_RISCV       = 243,  /* RISC-V */
};

/**
 * ELF identification indexes
 */
enum {
	EI_MAG0       = 0,
	EI_MAG1       = 1,
	EI_MAG2       = 2,
	EI_MAG3       = 3,
	EI_CLASS      = 4,   /* File class */
	EI_DATA       = 5,   /* Data encoding */
	EI_VERSION    = 6,   /* File version */
	EI_OSABI      = 7,
	EI_ABIVERSION = 8,
	EI_PAD        = 9,   /* Start of padding bytes */
	EI_NIDENT     = 16,  /* ELF identification table size */
};

/**
 * ELF magic number
 */
enum {
	ELFMAG0 = 0x7f,
	ELFMAG1 = 'E',
	ELFMAG2 = 'L',
	ELFMAG3 = 'F',
};

/**
 * ELF file classes
 */
enum elf_class {
	ELFCLASSNONE = 0,
	ELFCLASS32   = 1,
	ELFCLASS64   = 2,
};

/**
 * ELF data encoding types
 */
enum elf_data_encoding {
	ELFDATANONE = 0,
	ELFDATA2LSB = 1,  /* Least significant byte first (little endian) */
	ELFDATA2MSB = 2,  /* Most signigicant byte first (big endian) */
};

/**
 * ELF section types
 */
enum elf_section_type {
	SHT_NULL     = 0,
	SHT_PROGBITS = 1,
	SHT_SYMTAB   = 2,
	SHT_STRTAB   = 3,
	SHT_RELA     = 4,
	SHT_HASH     = 5,
	SHT_DYNAMIC  = 6,
	SHT_NOTE     = 7,
	SHT_NOBITS   = 8,
	SHT_REL      = 9,
	SHT_SHLIB    = 10,
	SHT_DYNSYM   = 11,
};

enum {
	SHT_LOOS     = 0x60000000,
	SHT_HIOS     = 0x6fffffff,
	SHT_LOPROC   = 0x70000000,
	SHT_HIPROC   = 0x7fffffff,
	SHT_LOUSER   = 0x80000000,
	SHT_HIUSER   = 0xffffffff,
};

/**
 * ELF section flags
 */
enum {
	SHF_WRITE     = 0x1,
	SHF_ALLOC     = 0x2,
	SHF_EXECINSTR = 0x4,
	SHF_TLS       = 0x400,
	SHF_MASKPROC  = 0xf0000000,
};

/** Functions for decomposing elf_symbol.st_info into binding and type */
static inline uint8_t elf_st_bind(uint8_t info)
{
	return info >> 4;
}

static inline uint8_t elf_st_type(uint8_t info)
{
	return info & 0x0f;
}

static inline uint8_t elf_st_info(uint8_t bind, uint8_t type)
{
	return (bind << 4) | (type & 0x0f);
}

/**
 * Symbol binding
 */
enum elf_symbol_binding {
	STB_LOCAL  = 0,
	STB_GLOBAL = 1,
	STB_WEAK   = 2,
};

enum {
	STB_LOPROC = 13,
	STB_HIPROC = 15,
};

/**
 * Symbol types
 */
enum elf_symbol_type {
	STT_NOTYPE  = 0,
	STT_OBJECT  = 1,
	STT_FUNC    = 2,
	STT_SECTION = 3,
	STT_FILE    = 4,
	STT_TLS     = 6,
};

enum {
	STT_LOPROC  = 13,
	STT_HIPROC  = 15,
};

/**
 * Program segment types
 */
enum elf_segment_type {
	PT_NULL         = 0,
	PT_LOAD         = 1,
	PT_DYNAMIC      = 2,
	PT_INTERP       = 3,
	PT_NOTE         = 4,
	PT_SHLIB        = 5,
	PT_PHDR         = 6,
	PT_TLS          = 7,

	PT_GNU_EH_FRAME = 0x6474e550,
	PT_GNU_STACK    = 0x6474e551,
	PT_GNU_RELRO    = 0x6474e552,
};

enum {
	PT_LOOS   = 0x60000000,
	PT_HIOS   = 0x6fffffff,
	PT_LOPROC = 0x70000000,
	PT_HIPROC = 0x7fffffff,
};

/**
 * Program segment attributes.
 */
enum elf_segment_access {
	PF_X = 1,
	PF_W = 2,
	PF_R = 4,
};

/**
 * Dynamic array tags
 */
enum elf_dynamic_tag {
	DT_NULL     = 0,
	DT_NEEDED   = 1,
	DT_PLTRELSZ = 2,
	DT_PLTGOT   = 3,
	DT_HASH     = 4,
	DT_STRTAB   = 5,
	DT_SYMTAB   = 6,
	DT_RELA     = 7,
	DT_RELASZ   = 8,
	DT_RELAENT  = 9,
	DT_STRSZ    = 10,
	DT_SYMENT   = 11,
	DT_INIT     = 12,
	DT_FINI     = 13,
	DT_SONAME   = 14,
	DT_RPATH    = 15,
	DT_SYMBOLIC = 16,
	DT_REL      = 17,
	DT_RELSZ    = 18,
	DT_RELENT   = 19,
	DT_PLTREL   = 20,
	DT_DEBUG    = 21,
	DT_TEXTREL  = 22,
	DT_JMPREL   = 23,
	DT_BIND_NOW = 24,
	DT_LOPROC   = 0x70000000,
	DT_HIPROC   = 0x7fffffff,
};

/**
 * Special section indexes
 */
enum {
	SHN_UNDEF     = 0,
	SHN_LORESERVE = 0xff00,
	SHN_LOPROC    = 0xff00,
	SHN_HIPROC    = 0xff1f,
	SHN_ABS       = 0xfff1,
	SHN_COMMON    = 0xfff2,
	SHN_HIRESERVE = 0xffff,
};

/**
 * Special symbol table index
 */
enum {
	STN_UNDEF = 0,
};

/**
 * ELF data types
 *
 * These types are found to be identical in both 32-bit and 64-bit
 * ELF object file specifications. They are the only types used
 * in ELF header.
 */
typedef uint64_t elf_xword;
typedef int64_t elf_sxword;
typedef uint32_t elf_word;
typedef int32_t elf_sword;
typedef uint16_t elf_half;

/**
 * 32-bit ELF data types.
 *
 * These types are specific for 32-bit format.
 */
typedef uint32_t elf32_addr;
typedef uint32_t elf32_off;

/**
 * 64-bit ELF data types.
 *
 * These types are specific for 64-bit format.
 */
typedef uint64_t elf64_addr;
typedef uint64_t elf64_off;

/** ELF header */
struct elf32_header {
	uint8_t e_ident[EI_NIDENT];
	elf_half e_type;
	elf_half e_machine;
	elf_word e_version;
	elf32_addr e_entry;
	elf32_off e_phoff;
	elf32_off e_shoff;
	elf_word e_flags;
	elf_half e_ehsize;
	elf_half e_phentsize;
	elf_half e_phnum;
	elf_half e_shentsize;
	elf_half e_shnum;
	elf_half e_shstrndx;
};

struct elf64_header {
	uint8_t e_ident[EI_NIDENT];
	elf_half e_type;
	elf_half e_machine;
	elf_word e_version;
	elf64_addr e_entry;
	elf64_off e_phoff;
	elf64_off e_shoff;
	elf_word e_flags;
	elf_half e_ehsize;
	elf_half e_phentsize;
	elf_half e_phnum;
	elf_half e_shentsize;
	elf_half e_shnum;
	elf_half e_shstrndx;
};

/**
 * ELF segment header.
 * Segments headers are also known as program headers.
 */
struct elf32_segment_header {
	elf_word p_type;
	elf32_off p_offset;
	elf32_addr p_vaddr;
	elf32_addr p_paddr;
	elf_word p_filesz;
	elf_word p_memsz;
	elf_word p_flags;
	elf_word p_align;
};

struct elf64_segment_header {
	elf_word p_type;
	elf_word p_flags;
	elf64_off p_offset;
	elf64_addr p_vaddr;
	elf64_addr p_paddr;
	elf_xword p_filesz;
	elf_xword p_memsz;
	elf_xword p_align;
};

/**
 * ELF section header
 */
struct elf32_section_header {
	elf_word sh_name;
	elf_word sh_type;
	elf_word sh_flags;
	elf32_addr sh_addr;
	elf32_off sh_offset;
	elf_word sh_size;
	elf_word sh_link;
	elf_word sh_info;
	elf_word sh_addralign;
	elf_word sh_entsize;
};

struct elf64_section_header {
	elf_word sh_name;
	elf_word sh_type;
	elf_xword sh_flags;
	elf64_addr sh_addr;
	elf64_off sh_offset;
	elf_xword sh_size;
	elf_word sh_link;
	elf_word sh_info;
	elf_xword sh_addralign;
	elf_xword sh_entsize;
};

/**
 * ELF symbol table entry
 */
struct elf32_symbol {
	elf_word st_name;
	elf32_addr st_value;
	elf_word st_size;
	uint8_t st_info;
	uint8_t st_other;
	elf_half st_shndx;
};

struct elf64_symbol {
	elf_word st_name;
	uint8_t st_info;
	uint8_t st_other;
	elf_half st_shndx;
	elf64_addr st_value;
	elf_xword st_size;
};

/*
 * ELF note segment entry
 */
struct elf32_note {
	elf_word namesz;
	elf_word descsz;
	elf_word type;
};

/*
 * NOTE: namesz, descsz and type should be 64-bits wide (elf_xword)
 * per the 64-bit ELF spec. The Linux kernel however screws up and
 * defines them as Elf64_Word, which is 32-bits wide(!). We are trying
 * to make our core files compatible with Linux GDB target so we copy
 * the blunder here.
 */
struct elf64_note {
	elf_word namesz;
	elf_word descsz;
	elf_word type;
};

/**
 * Dynamic structure
 */
struct elf32_dyn {
	elf_sword d_tag;
	union {
		elf_word d_val;
		elf32_addr d_ptr;
	} d_un;
};

struct elf64_dyn {
	elf_sxword d_tag;
	union {
		elf_xword d_val;
		elf64_addr d_ptr;
	} d_un;
};

struct elf32_rel {
	elf32_addr r_offset;
	elf_word r_info;
};

struct elf32_rela {
	elf32_addr r_offset;
	elf_word r_info;
	elf_sword r_addend;
};

struct elf64_rel {
	elf64_addr r_offset;
	elf_xword r_info;
};

struct elf64_rela {
	elf64_addr r_offset;
	elf_xword r_info;
	elf_sxword r_addend;
};

#define ELF32_R_SYM(i) ((i) >> 8)
#define ELF32_R_TYPE(i) ((unsigned char)(i))

#define ELF64_R_SYM(i) ((i) >> 32)
#define ELF64_R_TYPE(i) ((i) & 0xffffffffL)

#ifdef __32_BITS__
typedef struct elf32_header elf_header_t;
typedef struct elf32_segment_header elf_segment_header_t;
typedef struct elf32_section_header elf_section_header_t;
typedef struct elf32_symbol elf_symbol_t;
typedef struct elf32_note elf_note_t;
typedef struct elf32_dyn elf_dyn_t;
typedef struct elf32_rel elf_rel_t;
typedef struct elf32_rela elf_rela_t;
#define ELF_R_TYPE(i)  ELF32_R_TYPE(i)
#endif

#ifdef __64_BITS__
typedef struct elf64_header elf_header_t;
typedef struct elf64_segment_header elf_segment_header_t;
typedef struct elf64_section_header elf_section_header_t;
typedef struct elf64_symbol elf_symbol_t;
typedef struct elf64_note elf_note_t;
typedef struct elf64_dyn elf_dyn_t;
typedef struct elf64_rel elf_rel_t;
typedef struct elf64_rela elf_rela_t;
#define ELF_R_TYPE(i)  ELF64_R_TYPE(i)
#endif

#endif

/** @}
 */
