/*
 * Copyright (c) 2006 Martin Decky
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

#ifndef BOOT_mips32_ARCH_H_
#define BOOT_mips32_ARCH_H_

#define PAGE_WIDTH  14
#define PAGE_SIZE   (1 << PAGE_WIDTH)

#if defined(MACHINE_msim)
#define CPUMAP_OFFSET    0x00001000
#define STACK_OFFSET     0x00002000
#define BOOTINFO_OFFSET  0x00003000
#define BOOT_OFFSET      0x00100000
#define LOADER_OFFSET    0x1fc00000

#define MSIM_VIDEORAM_ADDRESS  0xb0000000
#define MSIM_DORDER_ADDRESS    0xb0000100
#endif

#if defined(MACHINE_lmalta) || defined(MACHINE_bmalta)
#define CPUMAP_OFFSET    0x00100000
#define STACK_OFFSET     0x00101000
#define BOOTINFO_OFFSET  0x00102000
#define BOOT_OFFSET      0x00200000
#define LOADER_OFFSET    0x00103000

#define YAMON_SUBR_BASE		PA2KA(0x1fc00500)
#define YAMON_SUBR_PRINT_COUNT	(YAMON_SUBR_BASE + 0x4)
#endif

#ifndef __ASSEMBLER__
#define PA2KA(addr)    (((uintptr_t) (addr)) + 0x80000000)
#define PA2KSEG(addr)  (((uintptr_t) (addr)) + 0xa0000000)
#define KA2PA(addr)    (((uintptr_t) (addr)) - 0x80000000)
#define KSEG2PA(addr)  (((uintptr_t) (addr)) - 0xa0000000)
#else
#define PA2KA(addr)    ((addr) + 0x80000000)
#define KSEG2PA(addr)  ((addr) - 0xa0000000)
#endif

#endif
