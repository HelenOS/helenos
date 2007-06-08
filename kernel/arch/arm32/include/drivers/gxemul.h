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

/** @addtogroup arm32gxemul GXemul
 *  @brief GXemul machine specific parts.
 *  @ingroup arm32
 * @{
 */
/** @file
 *  @brief GXemul peripheries drivers declarations.
 */

#ifndef KERN_arm32_GXEMUL_H_
#define KERN_arm32_GXEMUL_H_

#include <console/chardev.h>

/** Last interrupt number (beginning from 0) whose status is probed 
 * from interrupt controller
 */
#define GXEMUL_IRQC_MAX_IRQ		8

/** Timer frequency */
#define GXEMUL_TIMER_FREQ		100

/** Struct containing mappings of gxemul HW devices into kernel part 
 *  of virtual address space.
 */
typedef struct {
	uintptr_t videoram;
	uintptr_t kbd;
	uintptr_t rtc;
	uintptr_t rtc_freq;
	uintptr_t rtc_ack;
	uintptr_t irqc;
	uintptr_t irqc_mask;
	uintptr_t irqc_unmask;
} gxemul_hw_map_t;

extern void gxemul_hw_map_init(void);
extern void gxemul_console_init(devno_t devno);
extern void gxemul_release_console(void);
extern void gxemul_grab_console(void);
extern void gxemul_timer_irq_start(void);
extern void gxemul_debug_putc(char ch);
extern void gxemul_cpu_halt(void);
extern void gxemul_irq_exception(int exc_no, istate_t *istate);
extern size_t gxemul_get_memory_size(void);
extern uintptr_t gxemul_get_fb_address(void);

#endif

/** @}
 */
