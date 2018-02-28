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

/** @addtogroup drvi8042
 * @{
 */

/** @file
 * @brief i8042 driver DDF bits.
 */

#include <inttypes.h>
#include <libarch/config.h>
#include <ddf/driver.h>
#include <device/hw_res_parsed.h>
#include <errno.h>
#include <str_error.h>
#include <ddf/log.h>
#include <stdio.h>
#include "i8042.h"

/** Get address of I/O registers.
 *
 * @param[in]  dev            Device asking for the addresses.
 * @param[out] p_io_reg       Pointer to register range.
 * @param[out] kbd_irq        Primary port IRQ.
 * @param[out] mouse_irq      Auxiliary port IRQ.
 *
 * @return Error code.
 *
 */
static errno_t get_my_registers(ddf_dev_t *dev, addr_range_t *p_io_reg,
    int *kbd_irq, int *mouse_irq)
{
	assert(dev);

	async_sess_t *parent_sess = ddf_dev_parent_sess_get(dev);
	if (parent_sess == NULL)
		return ENOMEM;

	hw_res_list_parsed_t hw_resources;
	hw_res_list_parsed_init(&hw_resources);
	const errno_t ret = hw_res_get_list_parsed(parent_sess, &hw_resources, 0);
	if (ret != EOK)
		return ret;

	if ((hw_resources.irqs.count != 2) ||
	    (hw_resources.io_ranges.count != 1)) {
		hw_res_list_parsed_clean(&hw_resources);
		return EINVAL;
	}

	if (p_io_reg)
		*p_io_reg = hw_resources.io_ranges.ranges[0];

	if (kbd_irq)
		*kbd_irq = hw_resources.irqs.irqs[0];

	if (mouse_irq)
		*mouse_irq = hw_resources.irqs.irqs[1];

	hw_res_list_parsed_clean(&hw_resources);
	return EOK;
}

/** Initialize a new ddf driver instance of i8042 driver
 *
 * @param[in] device DDF instance of the device to initialize.
 *
 * @return Error code.
 *
 */
static errno_t i8042_dev_add(ddf_dev_t *device)
{
	addr_range_t io_regs;
	int kbd = 0;
	int mouse = 0;
	errno_t rc;

	if (!device)
		return EINVAL;

	rc = get_my_registers(device, &io_regs, &kbd, &mouse);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed to get registers: %s.",
		    str_error(rc));
		return rc;
	}

	ddf_msg(LVL_DEBUG,
	    "I/O regs at %p (size %zuB), IRQ kbd %d, IRQ mouse %d.",
	    RNGABSPTR(io_regs), RNGSZ(io_regs), kbd, mouse);

	i8042_t *i8042 = ddf_dev_data_alloc(device, sizeof(i8042_t));
	if (i8042 == NULL) {
		ddf_msg(LVL_ERROR, "Out of memory.");
		return ENOMEM;
	}

	rc = i8042_init(i8042, &io_regs, kbd, mouse, device);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed to initialize i8042 driver: %s.",
		    str_error(rc));
		return rc;
	}

	ddf_msg(LVL_NOTE, "Controlling '%s' (%" PRIun ").",
	    ddf_dev_get_name(device), ddf_dev_get_handle(device));
	return EOK;
}

/** DDF driver operations. */
static driver_ops_t i8042_driver_ops = {
	.dev_add = i8042_dev_add,
};

/** DDF driver. */
static driver_t i8042_driver = {
	.name = NAME,
	.driver_ops = &i8042_driver_ops
};

int main(int argc, char *argv[])
{
	printf("%s: HelenOS PS/2 driver.\n", NAME);
	ddf_log_init(NAME);

	return ddf_driver_main(&i8042_driver);
}

/**
 * @}
 */
