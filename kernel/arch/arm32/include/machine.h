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
 *  @brief Declarations of machine specific functions.
 *
 *  These functions enable to differentiate more kinds of ARM emulators
 *  or CPUs. It's the same concept as "arch" functions on the architecture
 *  level.
 */

#ifndef KERN_arm32_MACHINE_H_
#define KERN_arm32_MACHINE_H_

#include <console/console.h>
#include <arch/types.h>
#include <arch/exception.h>
#include <arch/drivers/gxemul.h>


/** Initializes console.
 *
 * @param devno Console device number.
 */
extern void machine_console_init(devno_t devno);


/** Acquire console back for kernel. */
extern void machine_grab_console(void);


/** Return console to userspace. */
extern void machine_release_console(void);


/** Maps HW devices to the kernel address space using #hw_map. */
extern void machine_hw_map_init(void);


/** Starts timer. */
extern void machine_timer_irq_start(void);


/** Halts CPU. */
extern void machine_cpu_halt(void);


/** Returns size of available memory.
 *
 *  @return Size of available memory.
 */
extern size_t machine_get_memory_size(void);


/** Prints a character. 
 *
 *  @param ch Character to be printed.
 */
extern void machine_debug_putc(char ch);


/** Interrupt exception handler.
 *
 * @param exc_no Interrupt exception number.
 * @param istate Saved processor state.
 */
extern void machine_irq_exception(int exc_no, istate_t *istate);


/** Returns address of framebuffer device.
 *
 *  @return Address of framebuffer device.
 */
extern uintptr_t machine_get_fb_address(void);


#ifdef MACHINE_gxemul
	#define machine_console_init(devno)            gxemul_console_init(devno)
	#define machine_hw_map_init                    gxemul_hw_map_init
	#define machine_timer_irq_start                gxemul_timer_irq_start
	#define machine_cpu_halt                       gxemul_cpu_halt
	#define machine_get_memory_size                gxemul_get_memory_size
	#define machine_debug_putc(ch)                 gxemul_debug_putc(ch)
	#define machine_irq_exception(exc_no, istate)  gxemul_irq_exception(exc_no, istate)
	#define machine_get_fb_address                 gxemul_get_fb_address
#endif

#endif

/** @}
 */
