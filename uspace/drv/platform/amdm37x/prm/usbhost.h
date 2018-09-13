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

/** @addtogroup amdm37x
 * @{
 */
/** @file
 * @brief Clock Control Clock Management IO register structure.
 */
#ifndef AMDM37X_PRM_CLOCK_CONTROL_H
#define AMDM37X_PRM_CLOCK_CONTROL_H

#include <ddi.h>
#include <macros.h>

/* AM/DM37x TRM p.536 and p.589 */
#define CLOCK_CONTROL_CM_BASE_ADDRESS  0x48307400
#define CLOCK_CONTROL_CM_SIZE  8192

/** Clock control PRM register map
 *
 * Periph DPLL == DPLL4
 * Core DPLL == DPLL3
 */
typedef struct {
	PADD32(22);
	ioport32_t rm_rstst;
#define USBHOST_PRM_RM_RSTST_COREDOMAINWKUP_RST_FLAG   (1 << 3)
#define USBHOST_PRM_RM_RSTST_DOMAINWKUP_RST_FLAG   (1 << 2)
#define USBHOST_PRM_RM_RSTST_GLOBALWARM_RST_FLAG   (1 << 1)
#define USBHOST_PRM_RM_RSTST_GLOBALCOLD_RST_FLAG   (1 << 0)

	PADD32(18);
	ioport32_t pm_wken;
#define USBHOST_PRM_PM_WKEN_EN_USBHOST_FLAG   (1 << 0)

	ioport32_t pm_mpugrpsel;
#define USBHOST_PRM_PM_MPUGRPSEL_GRPSEL_USBHOST_FLAG   (1 << 0)

	ioport32_t pm_iva2grpsel;
#define USBHOST_PRM_PM_IVA2GRPSEL_GRPSEL_USBHOST_FLAG   (1 << 0)

	PADD32(1);
	ioport32_t pm_wkst;
#define USBHOST_PRM_PM_WKST_ST_USBHOST_FLAG   (1 << 0)

	PADD32(5);
	ioport32_t pm_wkdep;
#define USBHOST_PRM_PM_WKDEP_EN_WKUP_FLAG   (1 << 4)
#define USBHOST_PRM_PM_WKDEP_EN_IVA2_FLAG   (1 << 2)
#define USBHOST_PRM_PM_WKDEP_EN_MPU_FLAG   (1 << 1)
#define USBHOST_PRM_PM_WKDEP_EN_CORE_FLAG   (1 << 0)

	PADD32(5);
	ioport32_t pm_pwstctrl;
#define USBHOST_PRM_PM_PWSTCTRL_MEMONSTATE_MASK   (0x3 << 16)
#define USBHOST_PRM_PM_PWSTCTRL_MEMONSTATE_ALWAYS_ON   (0x3 << 16)
#define USBHOST_PRM_PM_PWSTCTRL_MEMRETSTATE_FLAG   (1 << 8)
#define USBHOST_PRM_PM_PWSTCTRL_SAVEANDRESTORE_FLAG   (1 << 4)
#define USBHOST_PRM_PM_PWSTCTRL_LOGICRESTATE_FLAG   (1 << 2)
#define USBHOST_PRM_PM_PWSTCTRL_POWERSTATE_MASK   (0x3 << 0)
#define USBHOST_PRM_PM_PWSTCTRL_POWERSTATE_OFF   (0x0 << 0)
#define USBHOST_PRM_PM_PWSTCTRL_POWERSTATE_RETENTION   (0x1 << 0)
#define USBHOST_PRM_PM_PWSTCTRL_POWERSTATE_ON   (0x3 << 0)

	const ioport32_t pm_pwstst;
#define USBHOST_PRM_PM_PWSTST_INTRANSITION_FLAG   (1 << 20)
#define USBHOST_PRM_PM_PWSTST_POWERSTATEST_MASK   (0x3 << 0)
#define USBHOST_PRM_PM_PWSTST_POWERSTATEST_OFF   (0x0 << 0)
#define USBHOST_PRM_PM_PWSTST_POWERSTATEST_RETENTION  (0x1 << 0)
#define USBHOST_PRM_PM_PWSTST_POWERSTATEST_INACTIVE  (0x2 << 0)
#define USBHOST_PRM_PM_PWSTST_POWERSTATEST_ON  (0x3 << 0)

	ioport32_t pm_prepwstst;
#define USBHOST_PRM_PM_PREPWSTST_LASTPOWERSTATEENTERED_MASK   (0x3 << 0)
#define USBHOST_PRM_PM_PREPWSTST_LASTPOWERSTATEENTERED_OFF   (0x0 << 0)
#define USBHOST_PRM_PM_PREPWSTST_LASTPOWERSTATEENTERED_RETENTION  (0x1 << 0)
#define USBHOST_PRM_PM_PREPWSTST_LASTPOWERSTATEENTERED_INACTIVE  (0x2 << 0)
#define USBHOST_PRM_PM_PREPWSTST_LASTPOWERSTATEENTERED_ON  (0x3 << 0)

} usbhost_prm_regs_t;

#endif
/**
 * @}
 */
