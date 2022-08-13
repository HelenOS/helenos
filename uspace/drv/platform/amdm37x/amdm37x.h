/*
 * SPDX-FileCopyrightText: 2012 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup amdm37x
 * @{
 */
/** @file
 * @brief AM/DM 37x device.
 */
#ifndef AMDM37x_H
#define AMDM37x_H

#include "uhh.h"
#include "usbtll.h"

#include "cm/core.h"
#include "cm/clock_control.h"
#include "cm/usbhost.h"
#include "cm/mpu.h"
#include "cm/iva2.h"

#include "prm/clock_control.h"
#include "prm/global_reg.h"

#include <stdbool.h>

typedef struct {
	uhh_regs_t *uhh;
	tll_regs_t *tll;
	struct {
		mpu_cm_regs_t *mpu;
		iva2_cm_regs_t *iva2;
		core_cm_regs_t *core;
		clock_control_cm_regs_t *clocks;
		usbhost_cm_regs_t *usbhost;
	} cm;
	struct {
		clock_control_prm_regs_t *clocks;
		global_reg_prm_regs_t *global;
	} prm;
} amdm37x_t;

errno_t amdm37x_init(amdm37x_t *device, bool trace_io);
errno_t amdm37x_usb_tll_init(amdm37x_t *device);
void amdm37x_setup_dpll_on_autoidle(amdm37x_t *device);
void amdm37x_usb_clocks_set(amdm37x_t *device, bool enabled);

#endif
/**
 * @}
 */
