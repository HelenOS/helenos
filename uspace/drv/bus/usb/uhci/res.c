/*
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
/**
 * @addtogroup drvusbuhcihc
 * @{
 */
/**
 * @file
 * PCI related functions needed by the UHCI driver.
 */

#include <errno.h>
#include <assert.h>
#include <devman.h>
#include <device/hw_res_parsed.h>
#include <device/pci.h>

#include "res.h"

/** Call the PCI driver with a request to clear legacy support register
 *
 * @param[in] device Device asking to disable interrupts
 * @return Error code.
 */
int disable_legacy(ddf_dev_t *device)
{
	assert(device);

	async_sess_t *parent_sess = devman_parent_device_connect(
	    EXCHANGE_SERIALIZE, ddf_dev_get_handle(device), IPC_FLAG_BLOCKING);
	if (!parent_sess)
		return ENOMEM;

	/* See UHCI design guide page 45 for these values.
	 * Write all WC bits in USB legacy register */
	const int rc = pci_config_space_write_16(parent_sess, 0xc0, 0xaf00);

	async_hangup(parent_sess);
	return rc;
}

/**
 * @}
 */
