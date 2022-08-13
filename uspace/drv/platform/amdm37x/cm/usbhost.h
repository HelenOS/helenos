/*
 * SPDX-FileCopyrightText: 2012 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup amdm37x
 * @{
 */
/** @file
 * @brief USBHOST Clock Management IO register structure.
 */
#ifndef AMDM37x_USBHOST_CM_H
#define AMDM37x_USBHOST_CM_H

#include <macros.h>
#include <ddi.h>

/* AM/DM37x TRM p.447 */
#define USBHOST_CM_BASE_ADDRESS  0x48005400
#define USBHOST_CM_SIZE  8192

typedef struct {
	ioport32_t fclken;
#define USBHOST_CM_FCLKEN_EN_USBHOST1_FLAG  (1 << 0)
#define USBHOST_CM_FCLKEN_EN_USBHOST2_FLAG  (1 << 1)

	PADD32(3);
	ioport32_t iclken;
#define USBHOST_CM_ICLKEN_EN_USBHOST  (1 << 0)

	PADD32(3);
	const ioport32_t idlest;
#define USBHOST_CM_IDLEST_ST_USBHOST_STDBY_FLAG  (1 << 0)
#define USBHOST_CM_IDLEST_ST_USBHOST_IDLE_FLAG  (1 << 1)

	PADD32(3);
	ioport32_t autoidle;
#define USBHOST_CM_AUTOIDLE_AUTO_USBHOST_FLAG  (1 << 0)

	PADD32(4);
	ioport32_t sleepdep;
#define USBHOST_CM_SLEEPDEP_EN_MPU_FLAG  (1 << 1)
#define USBHOST_CM_SLEEPDEP_EN_IVA2_FLAG  (1 << 2)

	ioport32_t clkstctrl;
#define USBHOST_CM_CLKSTCTRL_CLKSTCTRL_USBHOST_MASK  (0x3 << 0)
#define USBHOST_CM_CLKSTCTRL_CLKSTCTRL_USBHOST_AUTO_DIS  (0x0 << 0)
#define USBHOST_CM_CLKSTCTRL_CLKSTCTRL_USBHOST_SUPERVISED_SLEEP  (0x1 << 0)
#define USBHOST_CM_CLKSTCTRL_CLKSTCTRL_USBHOST_SUPERVISED_WAKEUP  (0x2 << 0)
#define USBHOST_CM_CLKSTCTRL_CLKSTCTRL_USBHOST_AUTO_EN  (0x3 << 0)

	ioport32_t clkstst;
#define USBHOST_CM_CLKSTCTRL_CLKSTST_CLKACTIVITY_USBHOST  (1 << 0)
} usbhost_cm_regs_t;

#endif
/**
 * @}
 */
