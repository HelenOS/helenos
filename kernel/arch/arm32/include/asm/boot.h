/*
 * Copyright (c) 2007 Michal Kebrt
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

/** @addtogroup arm32	
 * @{
 */
/** @file
 *  @brief Initial kernel start.
 */

#ifndef KERN_arm32_ASM_BOOT_H_
#define KERN_arm32_ASM_BOOT_H_

/** Size of a temporary stack used for initial kernel start. */
#define TEMP_STACK_SIZE 0x100

#ifndef __ASM__

/** Kernel entry point.
 *
 * Implemented in assembly. Copies boot_bootinfo (declared as bootinfo in 
 * boot/arch/arm32/loader/main.c) to #bootinfo struct. Then jumps to
 * #arch_pre_main and #main_bsp.
 *
 * @param entry          Entry point address (not used).
 * @param boot_bootinfo  Struct holding information about loaded tasks.
 * @param bootinfo_size  Size of the bootinfo structure.
 */
extern void kernel_image_start(void *entry, void *boot_bootinfo,
    unsigned int bootinfo_size);

#endif

#endif

/** @}
 */
