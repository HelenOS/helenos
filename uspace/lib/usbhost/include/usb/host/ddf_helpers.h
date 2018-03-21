/*
 * Copyright (c) 2012 Jan Vesely
 * Copyright (c) 2018 Ondrej Hlavaty
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

/** @addtogroup libusbhost
 * @{
 */
/** @file
 *
 */

#ifndef LIBUSBHOST_HOST_DDF_HELPERS_H
#define LIBUSBHOST_HOST_DDF_HELPERS_H

#include <ddf/driver.h>
#include <ddf/interrupt.h>
#include <usb/usb.h>

#include <usb/host/hcd.h>
#include <usb/descriptor.h>


errno_t hcd_ddf_setup_hc(ddf_dev_t *, size_t);
void hcd_ddf_clean_hc(hc_device_t *);


device_t *hcd_ddf_fun_create(hc_device_t *, usb_speed_t);
void hcd_ddf_fun_destroy(device_t *);

errno_t hcd_ddf_setup_match_ids(device_t *, usb_standard_device_descriptor_t *);

errno_t hcd_ddf_enable_interrupt(hc_device_t *hcd, int);
errno_t hcd_ddf_get_registers(hc_device_t *hcd, hw_res_list_parsed_t *hw_res);

void hcd_ddf_gen_irq_handler(cap_call_handle_t icall_handle, ipc_call_t *call, ddf_dev_t *dev);

#endif

/**
 * @}
 */
