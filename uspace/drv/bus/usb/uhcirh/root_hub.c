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
/** @addtogroup drvusbuhcirh
 * @{
 */
/** @file
 * @brief UHCI root hub driver
 */
#include <errno.h>
#include <str_error.h>
#include <ddi.h>
#include <usb/debug.h>
#include <device/hw_res_parsed.h>

#include "root_hub.h"

/** Initialize UHCI root hub instance.
 *
 * @param[in] instance Driver memory structure to use.
 * @param[in] io_regs Range of I/O registers.
 * @param[in] rh Pointer to DDF instance of the root hub driver.
 * @return Error code.
 */
int uhci_root_hub_init(uhci_root_hub_t *instance, addr_range_t *io_regs,
    ddf_dev_t *rh)
{
	port_status_t *regs;

	assert(instance);
	assert(rh);

	/* Allow access to root hub port registers */
	assert(sizeof(*regs) * UHCI_ROOT_HUB_PORT_COUNT <= io_regs->size);

	int ret = pio_enable_range(io_regs, (void **) &regs);
	if (ret < 0) {
		usb_log_error(
		    "Failed(%d) to gain access to port registers at %p: %s.\n",
		    ret, RNGABSPTR(*io_regs), str_error(ret));
		return ret;
	}

	/* Initialize root hub ports */
	unsigned i = 0;
	for (; i < UHCI_ROOT_HUB_PORT_COUNT; ++i) {
		ret = uhci_port_init(
		    &instance->ports[i], &regs[i], i, ROOT_HUB_WAIT_USEC, rh);
		if (ret != EOK) {
			unsigned j = 0;
			for (;j < i; ++j)
				uhci_port_fini(&instance->ports[j]);
			return ret;
		}
	}

	return EOK;
}

/** Cleanup UHCI root hub instance.
 *
 * @param[in] instance Root hub structure to use.
 */
void uhci_root_hub_fini(uhci_root_hub_t* instance)
{
	assert(instance);
	unsigned i = 0;
	for (; i < UHCI_ROOT_HUB_PORT_COUNT; ++i) {
		uhci_port_fini(&instance->ports[i]);
	}
}

/**
 * @}
 */
