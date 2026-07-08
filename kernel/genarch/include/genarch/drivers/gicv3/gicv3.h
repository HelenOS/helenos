/*
 * Copyright (c) 2026 Jakub Kodym
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

/** @addtogroup kernel_genarch
 * @{
 */
/** @file
 * @brief ARM Generic Interrupt Controller, Architecture version 3.0, 64-bit.
 */

#ifndef KERN_GICV3_H_
#define KERN_GICV3_H_

#include <typedefs.h>
#include <stdbool.h>

#define LOWEST_SGI_ID   0
#define HIGHEST_SGI_ID  15

/**
 * A function that maps available core affinities to contiguous indices.
 * @param affinity The affinity value of a core.
 * @param proc_idx_out Pointer to store the mapped processor index.
 * @return EOK on success, ENOENT if the affinity is not supported.
 */
typedef errno_t (*aff_to_idx_t)(uint32_t affinity, uint32_t *proc_idx_out);

typedef struct {
	uintptr_t gicd_base;
	uintptr_t gicr_base;
	uint32_t inum_total;
	uintptr_t *gicr_base_addrs;
	uint32_t gicr_base_num;
	aff_to_idx_t aff_to_idx;
} gicv3_t;

void gicv3_init(gicv3_t *irqc, uintptr_t gicd_base, uintptr_t gicr_base,
    uintptr_t *gicr_base_addrs, uint32_t gicr_base_num,
    aff_to_idx_t aff_to_idx);
void gicv3_pe_init(gicv3_t *irqc);
void gicv3_irq_get_id(uint32_t *id);
uint32_t gicv3_irq_get_total(gicv3_t *irqc);
bool gicv3_is_spurious(uint32_t id);
void gicv3_end(uint32_t id);
void gicv3_enable(gicv3_t *irqc, uint32_t id);
void gicv3_disable(gicv3_t *irqc, uint32_t id);
void gicv3_send_sgi_all(uint32_t sgi_num);

#endif /* KERN_GICV3_H_ */

/** @}
 */
