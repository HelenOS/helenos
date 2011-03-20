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
	volatile uint32_t revision;
	volatile uint32_t control;
	volatile uint32_t command_status;
	volatile uint32_t interupt_enable;
	volatile uint32_t interrupt_disable;
	volatile uint32_t hcca;
	volatile uint32_t period_corrent;
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
} ohci_regs_t;
#endif
/**
 * @}
 */
