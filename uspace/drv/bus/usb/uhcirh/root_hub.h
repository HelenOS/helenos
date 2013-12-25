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
 * @brief UHCI driver
 */
#ifndef DRV_UHCI_ROOT_HUB_H
#define DRV_UHCI_ROOT_HUB_H

#include <ddf/driver.h>
#include <device/hw_res_parsed.h>

#include "port.h"

#define UHCI_ROOT_HUB_PORT_COUNT 2
#define ROOT_HUB_WAIT_USEC 250000 /* 250 miliseconds */

/** UHCI root hub drvier structure */
typedef struct root_hub {
	/** Ports provided by the hub */
	uhci_port_t ports[UHCI_ROOT_HUB_PORT_COUNT];
} uhci_root_hub_t;

int uhci_root_hub_init(uhci_root_hub_t *instance, addr_range_t *regs,
    ddf_dev_t *rh);

void uhci_root_hub_fini(uhci_root_hub_t *instance);
#endif
/**
 * @}
 */
