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
/** @addtogroup drvusbohci
 * @{
 */
/** @file
 * @brief OHCI driver
 */
#include <usb/usb.h>
#include "transfer_descriptor.h"

static unsigned dp[3] =
    { TD_STATUS_DP_IN, TD_STATUS_DP_OUT, TD_STATUS_DP_SETUP };
static unsigned togg[2] = { TD_STATUS_T_0, TD_STATUS_T_1 };

void td_init(td_t *instance,
    usb_direction_t dir, const void *buffer, size_t size, int toggle)
{
	assert(instance);
	bzero(instance, sizeof(td_t));
	instance->status = 0
	    | ((dp[dir] & TD_STATUS_DP_MASK) << TD_STATUS_DP_SHIFT)
	    | ((CC_NOACCESS2 & TD_STATUS_CC_MASK) << TD_STATUS_CC_SHIFT);
	if (toggle == 0 || toggle == 1) {
		instance->status |= togg[toggle] << TD_STATUS_T_SHIFT;
	}
	if (dir == USB_DIRECTION_IN) {
		instance->status |= TD_STATUS_ROUND_FLAG;
	}
	if (buffer != NULL) {
		assert(size != 0);
		instance->cbp = addr_to_phys(buffer);
		instance->be = addr_to_phys(buffer + size - 1);
	}
}
/**
 * @}
 */

