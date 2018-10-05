/*
 * Copyright (c) 2009 Vineeth Pillai
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

/** @addtogroup kernel_arm32_integratorcp
 *  @brief Integratorcp machine specific parts.
 *  @ingroup kernel_arm32
 * @{
 */
/** @file
 *  @brief Integratorcp peripheries drivers declarations.
 */

#ifndef KERN_arm32_icp_H_
#define KERN_arm32_icp_H_

#include <arch/machine_func.h>

/** Last interrupt number (beginning from 0) whose status is probed
 * from interrupt controller
 */
#define ICP_IRQC_MAX_IRQ  8
#define ICP_KBD_IRQ       3
#define ICP_TIMER_IRQ     6
#define ICP_UART0_IRQ     1

/** Timer frequency */
#define ICP_TIMER_FREQ  10000

#define ICP_UART			0x16000000
#define ICP_KBD				0x18000000
#define ICP_KBD_STAT			0x04
#define ICP_KBD_DATA			0x08
#define ICP_KBD_INTR_STAT		0x10
#define ICP_RTC				0x13000000
#define ICP_RTC1_LOAD_OFFSET		0x100
#define ICP_RTC1_READ_OFFSET		0x104
#define ICP_RTC1_CTL_OFFSET		0x108
#define ICP_RTC1_INTRCLR_OFFSET		0x10C
#define ICP_RTC1_INTRSTAT_OFFSET	0x114
#define ICP_RTC1_BGLOAD_OFFSET		0x118
#define ICP_RTC_CTL_VALUE		0x00E2
#define ICP_IRQC			0x14000000
#define ICP_IRQC_MASK_OFFSET		0xC
#define ICP_IRQC_UNMASK_OFFSET		0x8
#define ICP_FB				0x00800000
#define ICP_FB_FRAME			(ICP_FB >> 12)
#define ICP_FB_NUM_FRAME		512
#define ICP_VGA				0xC0000000
#define ICP_CMCR			0x10000000
#define ICP_SDRAM_MASK			0x1C
#define ICP_SDRAMCR_OFFSET		0x20

typedef struct {
	uintptr_t uart;
	uintptr_t kbd_ctrl;
	uintptr_t kbd_stat;
	uintptr_t kbd_data;
	uintptr_t kbd_intstat;
	uintptr_t rtc;
	uintptr_t rtc1_load;
	uintptr_t rtc1_read;
	uintptr_t rtc1_ctl;
	uintptr_t rtc1_intrclr;
	uintptr_t rtc1_intrstat;
	uintptr_t rtc1_bgload;
	uintptr_t irqc;
	uintptr_t irqc_mask;
	uintptr_t irqc_unmask;
	uintptr_t vga;
	uintptr_t cmcr;
	uintptr_t sdramcr;
} icp_hw_map_t;

extern void icp_init(void);
extern void icp_output_init(void);
extern void icp_input_init(void);
extern void icp_timer_irq_start(void);
extern void icp_cpu_halt(void);
extern void icp_irq_exception(unsigned int, istate_t *);
extern void icp_get_memory_extents(uintptr_t *, size_t *);
extern void icp_frame_init(void);
extern size_t icp_get_irq_count(void);
extern const char *icp_get_platform_name(void);

extern struct arm_machine_ops icp_machine_ops;

/** Size of IntegratorCP IRQ number range (starting from 0) */
#define ICP_IRQ_COUNT 8

#endif

/** @}
 */
