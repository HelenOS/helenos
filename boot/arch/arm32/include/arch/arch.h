/*
 * Copyright (c) 2010 Martin Decky
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

#ifndef BOOT_arm32_ARCH_H
#define BOOT_arm32_ARCH_H

#define PAGE_WIDTH  12
#define PAGE_SIZE   (1 << PAGE_WIDTH)

#define PTL0_ENTRIES     4096
#define PTL0_ENTRY_SIZE  4

/*
 * Address where the boot stage image starts (beginning of usable physical
 * memory).
 */
#ifdef MACHINE_gta02
#define BOOT_BASE	0x30008000
#elif defined MACHINE_beagleboardxm
#define BOOT_BASE	0x80000000
#elif defined MACHINE_beaglebone
#define BOOT_BASE       0x80000000
#elif defined MACHINE_raspberrypi
#define BOOT_BASE	0x00008000
#else
#define BOOT_BASE	0x00000000
#endif

#define BOOT_OFFSET	(BOOT_BASE + 0xa00000)

#ifdef MACHINE_beagleboardxm
#define PA_OFFSET 0
#elif defined MACHINE_beaglebone
#define PA_OFFSET 0
#else
#define PA_OFFSET 0x80000000
#endif

#ifndef __ASM__
#define PA2KA(addr)  (((uintptr_t) (addr)) + PA_OFFSET)
#else
#define PA2KA(addr)  ((addr) + PA_OFFSET)
#endif


#endif

/** @}
 */
