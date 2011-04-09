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
#include "endpoint_descriptor.h"

static unsigned direc[3] =
    { ED_STATUS_D_IN, ED_STATUS_D_OUT, ED_STATUS_D_TRANSFER };

void ed_init(ed_t *instance, endpoint_t *ep)
{
	assert(instance);
	bzero(instance, sizeof(ed_t));
	if (ep == NULL) {
		instance->status |= ED_STATUS_K_FLAG;
		return;
	}
	assert(ep);
	instance->status = 0
	    | ((ep->address & ED_STATUS_FA_MASK) << ED_STATUS_FA_SHIFT)
	    | ((ep->endpoint & ED_STATUS_EN_MASK) << ED_STATUS_EN_SHIFT)
	    | ((direc[ep->direction] & ED_STATUS_D_MASK) << ED_STATUS_D_SHIFT)
	    | ((ep->max_packet_size & ED_STATUS_MPS_MASK)
	        << ED_STATUS_MPS_SHIFT);

	if (ep->speed == USB_SPEED_LOW)
		instance->status |= ED_STATUS_S_FLAG;
	if (ep->transfer_type == USB_TRANSFER_ISOCHRONOUS)
		instance->status |= ED_STATUS_F_FLAG;

}

/**
 * @}
 */
