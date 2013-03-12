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

/** @addtogroup generic
 * @{
 */
/** @file
 */

#ifndef ABI_ELF_H_
#define ABI_ELF_H_

/**
 * Current ELF version
 */
#define EV_CURRENT  1

/**
 * ELF types
 */
#define ET_NONE    0       /* No type */
#define ET_REL     1       /* Relocatable file */
#define ET_EXEC    2       /* Executable */
#define ET_DYN     3       /* Shared object */
#define ET_CORE    4       /* Core */
#define ET_LOPROC  0xff00  /* Processor specific */
#define ET_HIPROC  0xffff  /* Processor specific */

/**
 * ELF machine types
 */
#define EM_NO           0   /* No machine */
#define EM_SPARC        2   /* SPARC */
#define EM_386          3   /* i386 */
#define EM_MIPS         8   /* MIPS RS3000 */
#define EM_MIPS_RS3_LE  10  /* MIPS RS3000 LE */
#define EM_PPC          20  /* PPC32 */
#define EM_PPC64        21  /* PPC64 */
#define EM_ARM          40  /* ARM */
#define EM_SPARCV9      43  /* SPARC64 */
#define EM_IA_64        50  /* IA-64 */
#define EM_X86_64       62  /* AMD64/EMT64 */

/**
 * ELF identification indexes
 */
#define EI_MAG0        0
#define EI_MAG1        1
#define EI_MAG2        2
#define EI_MAG3        3
#define EI_CLASS       4   /* File class */
#define EI_DATA        5   /* Data encoding */
#define EI_VERSION     6   /* File version */
#define EI_OSABI       7
#define EI_ABIVERSION  8
#define EI_PAD         9   /* Start of padding bytes */
#define EI_NIDENT      16  /* ELF identification table size */

/**
 * ELF magic number
 */
#define ELFMAG0  0x7f
#define ELFMAG1  'E'
#define ELFMAG2  'L'
#define ELFMAG3  'F'

/**
 * ELF file classes
 */
#define ELFCLASSNONE  0
#define ELFCLASS32    1
#define ELFCLASS64    2

/**
 * ELF data encoding types
 */
#define ELFDATANONE  0
#define ELFDATA2LSB  1  /* Least significant byte first (little endian) */
#define ELFDATA2MSB  2  /* Most signigicant byte first (big endian) */

/**
 * ELF section types
 */
#define SHT_NULL      0
#define SHT_PROGBITS  1
#define SHT_SYMTAB    2
#define SHT_STRTAB    3
#define SHT_RELA      4
#define SHT_HASH      5
#define SHT_DYNAMIC   6
#define SHT_NOTE      7
#define SHT_NOBITS    8
#define SHT_REL       9
#define SHT_SHLIB     10
#define SHT_DYNSYM    11
#define SHT_LOOS      0x60000000
#define SHT_HIOS      0x6fffffff
#define SHT_LOPROC    0x70000000
#define SHT_HIPROC    0x7fffffff
#define SHT_LOUSER    0x80000000
#define SHT_HIUSER    0xffffffff

/**
 * ELF section flags
 */
#define SHF_WRITE      0x1
#define SHF_ALLOC      0x2
#define SHF_EXECINSTR  0x4
#define SHF_TLS        0x400
#define SHF_MASKPROC   0xf0000000

/** Macros for decomposing elf_symbol.st_info into binging and type */
#define ELF_ST_BIND(i)     ((i) >> 4)
#define ELF_ST_TYPE(i)     ((i) & 0x0f)
#define ELF_ST_INFO(b, t)  (((b) << 4) + ((t) & 0x0f))

/**
 * Symbol binding
 */
#define STB_LOCAL   0
#define STB_GLOBAL  1
#define STB_WEAK    2
#define STB_LOPROC  13
#define STB_HIPROC  15

/**
 * Symbol types
 */
#define STT_NOTYPE   0
#define STT_OBJECT   1
#define STT_FUNC     2
#define STT_SECTION  3
#define STT_FILE     4
#define STT_LOPROC   13
#define STT_HIPROC   15

/**
 * Program segment types
 */
#define PT_NULL     0
#define PT_LOAD     1
#define PT_DYNAMIC  2
#define PT_INTERP   3
#define PT_NOTE     4
#define PT_SHLIB    5
#define PT_PHDR     6
#define PT_LOPROC   0x70000000
#define PT_HIPROC   0x7fffffff

/**
 * Program segment attributes.
 */
#define PF_X  1
#define PF_W  2
#define PF_R  4

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

#ifdef __32_BITS__
typedef struct elf32_header elf_header_t;
typedef struct elf32_segment_header elf_segment_header_t;
typedef struct elf32_section_header elf_section_header_t;
typedef struct elf32_symbol elf_symbol_t;
typedef struct elf32_note elf_note_t;
#endif

#ifdef __64_BITS__
typedef struct elf64_header elf_header_t;
typedef struct elf64_segment_header elf_segment_header_t;
typedef struct elf64_section_header elf_section_header_t;
typedef struct elf64_symbol elf_symbol_t;
typedef struct elf64_note elf_note_t;
#endif

#endif

/** @}
 */
