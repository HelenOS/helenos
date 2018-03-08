/*
 * Copyright (c) 2007 Pavel Jancik, Michal Kebrt
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

/** @addtogroup arm32mm
 * @{
 */
/** @file
 *  @brief Frame related declarations.
 */

#ifndef KERN_arm32_FRAME_H_
#define KERN_arm32_FRAME_H_

#define FRAME_WIDTH  12  /* 4KB frames */
#define FRAME_SIZE   (1 << FRAME_WIDTH)

#define FRAME_LOWPRIO  0

#ifndef __ASSEMBLER__

#define BOOT_PAGE_TABLE_SIZE     0x4000

#ifdef MACHINE_gta02

#define PHYSMEM_START_ADDR       0x30008000
#define BOOT_PAGE_TABLE_ADDRESS  0x30010000

#elif defined MACHINE_beagleboardxm

#define PHYSMEM_START_ADDR       0x80000000
#define BOOT_PAGE_TABLE_ADDRESS  0x80008000

#elif defined MACHINE_beaglebone

#define PHYSMEM_START_ADDR       0x80000000
#define BOOT_PAGE_TABLE_ADDRESS  0x80008000

#elif defined MACHINE_raspberrypi

#define PHYSMEM_START_ADDR       0x00000000
#define BOOT_PAGE_TABLE_ADDRESS  0x00010000

#else

#define PHYSMEM_START_ADDR       0x00000000
#define BOOT_PAGE_TABLE_ADDRESS  0x00008000

#endif

#define BOOT_PAGE_TABLE_START_FRAME     (BOOT_PAGE_TABLE_ADDRESS >> FRAME_WIDTH)
#define BOOT_PAGE_TABLE_SIZE_IN_FRAMES  (BOOT_PAGE_TABLE_SIZE >> FRAME_WIDTH)

extern void frame_low_arch_init(void);
extern void frame_high_arch_init(void);
extern void boot_page_table_free(void);
#define physmem_print()

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
