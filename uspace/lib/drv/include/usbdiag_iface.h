/*
 * Copyright (c) 2017 Petr Manek
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

/** @addtogroup libdrv
* @addtogroup usb
* @{
*/
/** @file
* @brief USB diagnostic device interface definition.
*/

#ifndef LIBDRV_USBDIAG_IFACE_H_
#define LIBDRV_USBDIAG_IFACE_H_

#include <async.h>
#include <usbhc_iface.h>
#include "ddf/driver.h"

#define USBDIAG_CATEGORY "usbdiag"

/** Milliseconds */
typedef unsigned long usbdiag_dur_t;

/** Test parameters. */
typedef struct usbdiag_test_params {
	usb_transfer_type_t transfer_type;
	size_t transfer_size;
	usbdiag_dur_t min_duration;
	bool validate_data;
} usbdiag_test_params_t;

/** Test results. */
typedef struct usbdiag_test_results {
	usbdiag_dur_t act_duration;
	uint32_t transfer_count;
	size_t transfer_size;
} usbdiag_test_results_t;

async_sess_t *usbdiag_connect(devman_handle_t);
void usbdiag_disconnect(async_sess_t *);

errno_t usbdiag_test_in(async_exch_t *,
    const usbdiag_test_params_t *, usbdiag_test_results_t *);
errno_t usbdiag_test_out(async_exch_t*,
    const usbdiag_test_params_t *, usbdiag_test_results_t *);

/** USB diagnostic device communication interface. */
typedef struct {
	errno_t (*test_in)(ddf_fun_t *,
	    const usbdiag_test_params_t *, usbdiag_test_results_t *);
	errno_t (*test_out)(ddf_fun_t*,
	    const usbdiag_test_params_t *, usbdiag_test_results_t *);
} usbdiag_iface_t;

#endif
/**
 * @}
 */
