/*
 * Copyright (c) 2015-2020 ARM Limited and Contributors.
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
 * @brief ARM Generic Interrupt Controller v3 distributor helper functions.
 */

#include <genarch/drivers/gicv3/tfa/gicv3_private.h>
#include <genarch/drivers/gicv3/tfa/gicv3_common_private.h>

/*
 * GIC Distributor functions for accessing the GIC registers
 * corresponding to a single interrupt ID. These functions use bitwise
 * operations or appropriate register accesses to modify or return
 * the bit-field corresponding the single interrupt ID.
 */

/*
 * Accessors to set the bit corresponding to interrupt ID
 * in GIC Distributor ICENABLER and ICENABLERE.
 */
void gicd_set_icenabler(uintptr_t base, unsigned int id)
{
	GICD_WRITE_BIT(ICENABLE, base, id);
}

/*
 * Accessors to set the bit corresponding to interrupt ID
 * in GIC Distributor ISENABLER and ISENABLERE.
 */
void gicd_set_isenabler(uintptr_t base, unsigned int id)
{
	GICD_WRITE_BIT(ISENABLE, base, id);
}

/*
 * GIC Distributor interface accessors for reading/writing entire registers
 */

/*
 * Accessors to read/write the GIC Distributor ICGFR and ICGFRE
 * corresponding to the interrupt ID, 16 interrupt IDs at a time.
 */

void gicd_write_icfgr(uintptr_t base, unsigned int id, unsigned int val)
{
	GICD_WRITE(ICFG, base, id, val);
}

/*
 * Accessors to read/write the GIC Distributor IGROUPR and IGROUPRE
 * corresponding to the interrupt ID, 32 interrupt IDs at a time.
 */

void gicd_write_igroupr(uintptr_t base, unsigned int id, unsigned int val)
{
	GICD_WRITE(IGROUP, base, id, val);
}

/*
 * Accessors to read/write the GIC Distributor IPRIORITYR and IPRIORITYRE
 * corresponding to the interrupt ID, 4 interrupt IDs at a time.
 */

void gicd_write_ipriorityr(uintptr_t base, unsigned int id, unsigned int val)
{
	GICD_WRITE(IPRIORITY, base, id, val);
}

/** @}
 */
