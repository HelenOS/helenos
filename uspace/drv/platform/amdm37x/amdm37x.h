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

/** @addtogroup amdm37xdrv
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
