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
/** @addtogroup drvusbuhci
 * @{
 */
/** @file
 * @brief UHCI driver
 */
#include <assert.h>
#include <errno.h>
#include <str_error.h>
#include <stdio.h>
#include <device/hw_res_parsed.h>

#include <usb/debug.h>

#include "root_hub.h"

/** Root hub initialization
 * @param[in] instance RH structure to initialize
 * @param[in] fun DDF function representing UHCI root hub
 * @param[in] reg_addr Address of root hub status and control registers.
 * @param[in] reg_size Size of accessible address space.
 * @return Error code.
 */
int
rh_init(rh_t *instance, ddf_fun_t *fun, addr_range_t *regs, uintptr_t reg_addr,
    size_t reg_size)
{
	assert(instance);
	assert(fun);

	/* Crop the PIO window to the absolute address range of UHCI I/O. */
	instance->pio_window.mem.base = 0;
	instance->pio_window.mem.size = 0;
	instance->pio_window.io.base = RNGABS(*regs);
	instance->pio_window.io.size = RNGSZ(*regs);

	/* Initialize resource structure */
	instance->resource_list.count = 1;
	instance->resource_list.resources = &instance->io_regs;

	instance->io_regs.type = IO_RANGE;
	instance->io_regs.res.io_range.address = reg_addr;
	instance->io_regs.res.io_range.size = reg_size;
	instance->io_regs.res.io_range.relative = true;
	instance->io_regs.res.io_range.endianness = LITTLE_ENDIAN;

	const int ret = ddf_fun_add_match_id(fun, "usb&uhci&root-hub", 100);
	if (ret != EOK) {
		usb_log_error("Failed to add root hub match id: %s\n",
		    str_error(ret));
	}
	return ret;
}
/**
 * @}
 */
