/*
 * Copyright (c) 2011 Jan Vesely
 * Copyright (c) 2011 Vojtech Horky
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
/** @addtogroup drvaudiosb16
 * @{
 */
/** @file
 * Main routines of Creative Labs SoundBlaster 16 driver
 */
#include <ddf/driver.h>
#include <ddf/interrupt.h>
#include <ddf/log.h>
#include <device/hw_res_parsed.h>
#include <devman.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <str_error.h>

#include "ddf_log.h"
#include "sb16.h"

#define NAME "sb16"

static int sb_add_device(ddf_dev_t *device);
static int sb_get_res(const ddf_dev_t *device, uintptr_t *sb_regs,
    size_t *sb_regs_size, uintptr_t *mpu_regs, size_t *mpu_regs_size,
    int *irq, int *dma8, int *dma16);
static int sb_enable_interrupts(ddf_dev_t *device);
/*----------------------------------------------------------------------------*/
static driver_ops_t sb_driver_ops = {
	.dev_add = sb_add_device,
};
/*----------------------------------------------------------------------------*/
static driver_t sb_driver = {
	.name = NAME,
	.driver_ops = &sb_driver_ops
};
//static ddf_dev_ops_t sb_ops = {};
/*----------------------------------------------------------------------------*/
/** Initializes global driver structures (NONE).
 *
 * @param[in] argc Nmber of arguments in argv vector (ignored).
 * @param[in] argv Cmdline argument vector (ignored).
 * @return Error code.
 *
 * Driver debug level is set here.
 */
int main(int argc, char *argv[])
{
	printf(NAME": HelenOS SB16 audio driver.\n");
	ddf_log_init(NAME, LVL_DEBUG2);
	return ddf_driver_main(&sb_driver);
}
/*----------------------------------------------------------------------------*/
static void irq_handler(ddf_dev_t *dev, ipc_callid_t iid, ipc_call_t *call)
{
	assert(dev);
	assert(dev->driver_data);
	sb16_interrupt(dev->driver_data);
}
/*----------------------------------------------------------------------------*/
/** Initializes a new ddf driver instance of SB16.
 *
 * @param[in] device DDF instance of the device to initialize.
 * @return Error code.
 */
static int sb_add_device(ddf_dev_t *device)
{
#define CHECK_RET_RETURN(ret, msg...) \
if (ret != EOK) { \
	ddf_log_error(msg); \
	return ret; \
} else (void)0

	assert(device);

	sb16_t *soft_state = ddf_dev_data_alloc(device, sizeof(sb16_t));
	int ret = soft_state ? EOK : ENOMEM;
	CHECK_RET_RETURN(ret, "Failed to allocate sb16 structure.");

	uintptr_t sb_regs = 0, mpu_regs = 0;
	size_t sb_regs_size = 0, mpu_regs_size = 0;
	int irq = 0, dma8 = 0, dma16 = 0;

	ret = sb_get_res(device, &sb_regs, &sb_regs_size, &mpu_regs,
	    &mpu_regs_size, &irq, &dma8, &dma16);
	CHECK_RET_RETURN(ret, "Failed to get resources: %s.", str_error(ret));

	const size_t irq_cmd_count = sb16_irq_code_size();
	irq_cmd_t irq_cmds[irq_cmd_count];
	irq_pio_range_t irq_ranges[1];
	sb16_irq_code((void*)sb_regs, dma8, dma16, irq_cmds, irq_ranges);

	irq_code_t irq_code = {
		.cmdcount = irq_cmd_count,
		.cmds = irq_cmds,
		.rangecount = 1,
		.ranges = irq_ranges
	};

	ret = register_interrupt_handler(device, irq, irq_handler, &irq_code);
	CHECK_RET_RETURN(ret,
	    "Failed to register irq handler: %s.", str_error(ret));

#define CHECK_RET_UNREG_DEST_RETURN(ret, msg...) \
if (ret != EOK) { \
	ddf_log_error(msg); \
	unregister_interrupt_handler(device, irq); \
	return ret; \
} else (void)0

	ret = sb_enable_interrupts(device);
	CHECK_RET_UNREG_DEST_RETURN(ret, "Failed to enable interrupts: %s.",
	    str_error(ret));

	ret = sb16_init_sb16(
	    soft_state, (void*)sb_regs, sb_regs_size, device, dma8, dma16);
	CHECK_RET_UNREG_DEST_RETURN(ret,
	    "Failed to init sb16 driver: %s.", str_error(ret));

	ret = sb16_init_mpu(soft_state, (void*)mpu_regs, mpu_regs_size);
	if (ret == EOK) {
		ddf_fun_t *mpu_fun =
		    ddf_fun_create(device, fun_exposed, "midi");
		if (mpu_fun) {
			ret = ddf_fun_bind(mpu_fun);
			if (ret != EOK)
				ddf_log_error(
				    "Failed to bind midi function: %s.",
				    str_error(ret));
		} else {
			ddf_log_error("Failed to create midi function.");
		}
	} else {
	    ddf_log_warning("Failed to init mpu driver: %s.", str_error(ret));
	}

	/* MPU state does not matter */
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int sb_get_res(const ddf_dev_t *device, uintptr_t *sb_regs,
    size_t *sb_regs_size, uintptr_t *mpu_regs, size_t *mpu_regs_size,
    int *irq, int *dma8, int *dma16)
{
	assert(device);

	async_sess_t *parent_sess =
	    devman_parent_device_connect(EXCHANGE_SERIALIZE, device->handle,
            IPC_FLAG_BLOCKING);
	if (!parent_sess)
		return ENOMEM;

	hw_res_list_parsed_t hw_res;
	hw_res_list_parsed_init(&hw_res);
	const int ret = hw_res_get_list_parsed(parent_sess, &hw_res, 0);
	async_hangup(parent_sess);
	if (ret != EOK) {
		return ret;
	}

	/* 1x IRQ, 1-2x DMA(8,16), 1-2x IO (MPU is separate). */
	if (hw_res.irqs.count != 1 ||
	   (hw_res.io_ranges.count != 1 && hw_res.io_ranges.count != 2) ||
	   (hw_res.dma_channels.count != 1 && hw_res.dma_channels.count != 2)) {
		hw_res_list_parsed_clean(&hw_res);
		return EINVAL;
	}

	if (irq)
		*irq = hw_res.irqs.irqs[0];

	if (dma8) {
		if (hw_res.dma_channels.channels[0] < 4) {
			*dma8 = hw_res.dma_channels.channels[0];
		} else {
			if (hw_res.dma_channels.count == 2 &&
			    hw_res.dma_channels.channels[1] < 4) {
				*dma8 = hw_res.dma_channels.channels[1];
			}
		}
	}

	if (dma16) {
		if (hw_res.dma_channels.channels[0] > 4) {
			*dma16 = hw_res.dma_channels.channels[0];
		} else {
			if (hw_res.dma_channels.count == 2 &&
			    hw_res.dma_channels.channels[1] > 4) {
				*dma16 = hw_res.dma_channels.channels[1];
			}
		}
	}


	if (hw_res.io_ranges.count == 1) {
		if (sb_regs)
			*sb_regs = hw_res.io_ranges.ranges[0].address;
		if (sb_regs_size)
			*sb_regs_size = hw_res.io_ranges.ranges[0].size;
	} else {
		const int sb =
		    (hw_res.io_ranges.ranges[0].size >= sizeof(sb16_regs_t))
		        ? 1 : 0;
		const int mpu = 1 - sb;
		if (sb_regs)
			*sb_regs = hw_res.io_ranges.ranges[sb].address;
		if (sb_regs_size)
			*sb_regs_size = hw_res.io_ranges.ranges[sb].size;
		if (mpu_regs)
			*sb_regs = hw_res.io_ranges.ranges[mpu].address;
		if (mpu_regs_size)
			*sb_regs_size = hw_res.io_ranges.ranges[mpu].size;
	}

	return EOK;
}
/*----------------------------------------------------------------------------*/
int sb_enable_interrupts(ddf_dev_t *device)
{
	async_sess_t *parent_sess =
	    devman_parent_device_connect(EXCHANGE_SERIALIZE, device->handle,
	    IPC_FLAG_BLOCKING);
	if (!parent_sess)
		return ENOMEM;

	bool enabled = hw_res_enable_interrupt(parent_sess);
	async_hangup(parent_sess);

	return enabled ? EOK : EIO;
}
/**
 * @}
 */
