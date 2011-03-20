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
/** @addtogroup drvusbohcihc
 * @{
 */
/** @file
 * @brief OHCI Host controller driver routines
 */
#include <errno.h>
#include <str_error.h>
#include <adt/list.h>
#include <libarch/ddi.h>

#include <usb/debug.h>
#include <usb/usb.h>
#include <usb/ddfiface.h>
#include <usb_iface.h>

#include "ohci_hc.h"

int ohci_hc_init(ohci_hc_t *instance, ddf_fun_t *fun,
    uintptr_t regs, size_t reg_size, bool interrupts)
{
	assert(instance);
	int ret = pio_enable((void*)regs, reg_size, (void**)&instance->registers);
	if (ret != EOK) {
		usb_log_error("Failed to gain access to device registers.\n");
		return ret;
	}
	instance->registers->interrupt_disable = 0;
	/* enable interrupt on root hub status change */
	instance->registers->interupt_enable |= IE_RHSC | IE_MIE;


	ohci_rh_init(&instance->rh, instance->registers);
	/* TODO: implement */
	/* TODO: register root hub */
	return EOK;
}
/*----------------------------------------------------------------------------*/
int ohci_hc_schedule(ohci_hc_t *instance, batch_t *batch)
{
	assert(instance);
	assert(batch);
	if (batch->target.address == instance->rh.address) {
		ohci_rh_request(&instance->rh, batch);
		return EOK;
	}
	/* TODO: implement */
	return EOK;
}
/*----------------------------------------------------------------------------*/
void ohci_hc_interrupt(ohci_hc_t *instance, uint16_t status)
{
	assert(instance);
	/* TODO: Check for interrupt cause */
	ohci_rh_interrupt(&instance->rh);
	/* TODO: implement */
}
/**
 * @}
 */
