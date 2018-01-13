/*
 * Copyright (c) 2012 Jan Vesely
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

#include <usb/host/hcd.h>
#include <usb/host/usb_bus.h>
#include <usb/usb.h>

#include <ddf/driver.h>
#include <ddf/interrupt.h>
#include <device/hw_res_parsed.h>

typedef errno_t (*driver_init_t)(hcd_t *, const hw_res_list_parsed_t *, bool);
typedef void (*driver_fini_t)(hcd_t *);
typedef errno_t (*claim_t)(ddf_dev_t *);
typedef errno_t (*irq_code_gen_t)(irq_code_t *, const hw_res_list_parsed_t *, int *);

typedef struct {
	hcd_ops_t ops;
	claim_t claim;
	usb_speed_t hc_speed;
	driver_init_t init;
	driver_fini_t fini;
	interrupt_handler_t *irq_handler;
	irq_code_gen_t irq_code_gen;
	const char *name;
} ddf_hc_driver_t;

errno_t hcd_ddf_add_hc(ddf_dev_t *device, const ddf_hc_driver_t *driver);

errno_t hcd_ddf_setup_hc(ddf_dev_t *device, usb_speed_t max_speed,
    size_t bw, bw_count_func_t bw_count);
void hcd_ddf_clean_hc(ddf_dev_t *device);
errno_t hcd_ddf_setup_root_hub(ddf_dev_t *device);

hcd_t *dev_to_hcd(ddf_dev_t *dev);

errno_t hcd_ddf_enable_interrupt(ddf_dev_t *device, int);
errno_t hcd_ddf_get_registers(ddf_dev_t *device, hw_res_list_parsed_t *hw_res);
errno_t hcd_ddf_setup_interrupts(ddf_dev_t *device,
    const hw_res_list_parsed_t *hw_res,
    interrupt_handler_t handler,
    errno_t (*gen_irq_code)(irq_code_t *, const hw_res_list_parsed_t *, int *),
    cap_handle_t *handle);
void ddf_hcd_gen_irq_handler(ipc_call_t *call, ddf_dev_t *dev);

#endif

/**
 * @}
 */
