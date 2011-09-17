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
#include <device/hw_res.h>
#include <devman.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>

//#include <str_error.h>

#include "ddf_log.h"
#include "sb16.h"

#define NAME "sb16"

static int sb_add_device(ddf_dev_t *device);
static int sb_get_res(const ddf_dev_t *device, uintptr_t *sb_regs,
    size_t *sb_regs_size, uintptr_t *mpu_regs, size_t *mpu_regs_size, int *irq);
/*----------------------------------------------------------------------------*/
static driver_ops_t sb_driver_ops = {
	.add_device = sb_add_device,
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
/** Initializes a new ddf driver instance of SB16.
 *
 * @param[in] device DDF instance of the device to initialize.
 * @return Error code.
 */
static int sb_add_device(ddf_dev_t *device)
{
	assert(device);
	uintptr_t sb_regs = 0, mpu_regs = 0;
	size_t sb_regs_size = 0, mpu_regs_size = 0;
	int irq = 0;
	int ret = sb_get_res(device, &sb_regs, &sb_regs_size, &mpu_regs,
	    &mpu_regs_size, &irq);

	return ret;
}
/*----------------------------------------------------------------------------*/
static int sb_get_res(const ddf_dev_t *device, uintptr_t *sb_regs,
    size_t *sb_regs_size, uintptr_t *mpu_regs, size_t *mpu_regs_size, int *irq)
{
	assert(device);
	assert(sb_regs);
	assert(sb_regs_size);
	assert(mpu_regs);
	assert(mpu_regs_size);
	assert(irq);

	async_sess_t *parent_sess =
	    devman_parent_device_connect(EXCHANGE_SERIALIZE, device->handle,
            IPC_FLAG_BLOCKING);
	if (!parent_sess)
		return ENOMEM;

	hw_resource_list_t hw_resources;
	const int rc = hw_res_get_resource_list(parent_sess, &hw_resources);
	if (rc != EOK) {
		async_hangup(parent_sess);
		return rc;
	}

	size_t i;
	for (i = 0; i < hw_resources.count; i++) {
		const hw_resource_t *res = &hw_resources.resources[i];
		switch (res->type) {
		case INTERRUPT:
			*irq = res->res.interrupt.irq;
			ddf_log_debug("Found interrupt: %d.\n", *irq);
			break;
		case IO_RANGE:
			ddf_log_debug("Found io: %" PRIx64" %zu.\n",
			    res->res.io_range.address, res->res.io_range.size);
			if (res->res.io_range.size >= sizeof(sb16_regs_t)) {
	                        *sb_regs = res->res.io_range.address;
		                *sb_regs_size = res->res.io_range.size;
			} else {
				*mpu_regs = res->res.io_range.address;
				*mpu_regs_size = res->res.io_range.size;
			}
			break;
		default:
			break;
		}
	}

	async_hangup(parent_sess);
	free(hw_resources.resources);
	return EOK;
}
/**
 * @}
 */

