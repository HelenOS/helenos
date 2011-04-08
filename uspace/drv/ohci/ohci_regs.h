/*
 * Copyright (c) 2011 Jan Vesely
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

/** @addtogroup drvusbohcihc
 * @{
 */
/** @file
 * @brief OHCI host controller register structure
 */
#ifndef DRV_OHCI_OHCI_REGS_H
#define DRV_OHCI_OHCI_REGS_H
#include <stdint.h>

typedef struct ohci_regs
{
	const volatile uint32_t revision;
	volatile uint32_t control;
#define C_CSBR_MASK (0x3)
#define C_CSBR_SHIFT (0)
#define C_CSBR_1_1 (0x0)
#define C_CSBR_1_2 (0x1)
#define C_CSBR_1_3 (0x2)
#define C_CSBR_1_4 (0x3)

#define C_PLE (1 << 2)
#define C_IE (1 << 3)
#define C_CLE (1 << 4)
#define C_BLE (1 << 5)

#define C_HCFS_MASK (0x3)
#define C_HCFS_SHIFT (6)
#define C_HCFS_RESET (0x0)
#define C_HCFS_OPERATIONAL (0x1)
#define C_HCFS_RESUME (0x2)
#define C_HCFS_SUSPEND (0x3)

#define C_IR (1 << 8)
#define C_RWC (1 << 9)
#define C_RWE (1 << 10)

	volatile uint32_t command_status;
#define CS_HCR (1 << 0)
#define CS_CLF (1 << 1)
#define CS_BLF (1 << 2)
#define CS_OCR (1 << 3)
#define CS_SOC_MASK (0x3)
#define CS_SOC_SHIFT (16)

	volatile uint32_t interrupt_status;
#define IS_SO (1 << 0)
#define IS_WDH (1 << 1)
#define IS_SF (1 << 2)
#define IS_RD (1 << 3)
#define IS_UE (1 << 4)
#define IS_FNO (1 << 5)
#define IS_RHSC (1 << 6)
#define IS_OC (1 << 30)

	volatile uint32_t interupt_enable;
#define IE_SO   (1 << 0)
#define IE_WDH  (1 << 1)
#define IE_SF   (1 << 2)
#define IE_RD   (1 << 3)
#define IE_UE   (1 << 4)
#define IE_FNO  (1 << 5)
#define IE_RHSC (1 << 6)
#define IE_OC   (1 << 30)
#define IE_MIE  (1 << 31)

	volatile uint32_t interrupt_disable;
	volatile uint32_t hcca;
	volatile uint32_t period_current;
	volatile uint32_t control_head;
	volatile uint32_t control_current;
	volatile uint32_t bulk_head;
	volatile uint32_t bulk_current;
	volatile uint32_t done_head;
	volatile uint32_t fm_interval;
	volatile uint32_t fm_remaining;
	volatile uint32_t fm_number;
	volatile uint32_t periodic_start;
	volatile uint32_t ls_threshold;
	volatile uint32_t rh_desc_a;
	volatile uint32_t rh_desc_b;
	volatile uint32_t rh_status;
	volatile uint32_t rh_port_status[];
} __attribute__((packed)) ohci_regs_t;
#endif
/**
 * @}
 */
