/*
 * Copyright (c) 2012 Jan Vesely
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

/** @addtogroup amdm37xdrvuhh
 * @{
 */
/** @file
 * @brief UHH IO register structure.
 */
#ifndef AMDM37x_UHH_H
#define AMDM37x_UHH_H

#include <macros.h>
#include <ddi.h>

#define AMDM37x_UHH_BASE_ADDRESS  0x48064000
#define AMDM37x_UHH_SIZE  1024

typedef struct {
	const ioport32_t revision;
#define UHH_REVISION_MINOR_MASK  0x0f
#define UHH_REVISION_MAJOR_MASK  0xf0

	PADD32(3);
	ioport32_t sysconfig;
#define UHH_SYSCONFIG_AUTOIDLE_FLAG  (1 << 0)
#define UHH_SYSCONFIG_SOFTRESET_FLAG  (1 << 1)
#define UHH_SYSCONFIG_ENWAKEUP_FLAG  (1 << 2)
#define UHH_SYSCONFIG_SIDLE_MODE_MASK  (0x3 << 3)
#define UHH_SYSCONFIG_SIDLE_MODE_FORCE  (0x0 << 3)
#define UHH_SYSCONFIG_SIDLE_MODE_NO  (0x1 << 3)
#define UHH_SYSCONFIG_SIDLE_MODE_SMART  (0x2 << 3)
#define UHH_SYSCONFIG_CLOCKACTIVITY_FLAG  (1 << 8)
#define UHH_SYSCONFIG_MIDLE_MODE_MASK  (0x3 << 12)
#define UHH_SYSCONFIG_MIDLE_MODE_FORCE  (0x0 << 12)
#define UHH_SYSCONFIG_MIDLE_MODE_NO  (0x1 << 12)
#define UHH_SYSCONFIG_MIDLE_MODE_SMART  (0x2 << 12)

	const ioport32_t sysstatus;
#define UHH_SYSSTATUS_RESETDONE_FLAG  (1 << 0)
#define UHH_SYSSTATUS_OHCI_RESETDONE_FLAG  (1 << 1)
#define UHH_SYSSTATUS_EHCI_RESETDONE_FLAG  (1 << 2)

	PADD32(10);
	ioport32_t hostconfig;
#define UHH_HOSTCONFIG_P1_ULPI_BYPASS_FLAG  (1 << 0)
#define UHH_HOSTCONFIG_AUTOPPD_ON_OVERCUR_EN_FLAG  (1 << 1)
#define UHH_HOSTCONFIG_ENA_INCR4_FLAG  (1 << 2)
#define UHH_HOSTCONFIG_ENA_INCR8_FLAG  (1 << 3)
#define UHH_HOSTCONFIG_ENA_INCR16_FLA  (1 << 4)
#define UHH_HOSTCONFIG_ENA_INCR_ALIGN_FLAG  (1 << 5)
#define UHH_HOSTCONFIG_P1_CONNECT_STATUS_FLAG  (1 << 8)
#define UHH_HOSTCONFIG_P2_CONNECT_STATUS_FLAG  (1 << 9)
#define UHH_HOSTCONFIG_P3_CONNECT_STATUS_FLAG  (1 << 10)
#define UHH_HOSTCONFIG_P2_ULPI_BYPASS_FLAG  (1 << 11)
#define UHH_HOSTCONFIG_P3_ULPI_BYPASS_FLAG  (1 << 12)

	ioport32_t debug_csr;
#define UHH_DEBUG_CSR_EHCI_FLADJ_MASK  (0x3f << 0)
#define UHH_DEBUG_CSR_EHCI_FLADJ(x)  ((x) & 0x3f)
#define UHH_DEBUG_CSR_EHCI_SIMULATION_MODE_FLAG  (1 << 6)
#define UHH_DEBUG_CSR_OHCI_CNTSEL_FLAG  (1 << 7)
#define UHH_DEBUG_CSR_OHCI_GLOBAL_SUSPEND_FLAG  (1 << 16)
#define UHH_DEBUG_CSR_OHCI_CCS1_FLAG  (1 << 17)
#define UHH_DEBUG_CSR_OHCI_CCS2_FLAG  (1 << 18)
#define UHH_DEBUG_CSR_OHCI_CCS3_FLAG  (1 << 19)

} uhh_regs_t;

#endif
/**
 * @}
 */
