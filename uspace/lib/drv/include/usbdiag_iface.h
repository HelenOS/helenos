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
#include "ddf/driver.h"

#define USBDIAG_CATEGORY "usbdiag"

/** Milliseconds */
typedef unsigned long usbdiag_dur_t;

async_sess_t *usbdiag_connect(devman_handle_t);
void usbdiag_disconnect(async_sess_t*);

int usbdiag_burst_intr_in(async_exch_t*, int, size_t, usbdiag_dur_t*);
int usbdiag_burst_intr_out(async_exch_t*, int, size_t, usbdiag_dur_t*);
int usbdiag_burst_bulk_in(async_exch_t*, int, size_t, usbdiag_dur_t*);
int usbdiag_burst_bulk_out(async_exch_t*, int, size_t, usbdiag_dur_t*);
int usbdiag_burst_isoch_in(async_exch_t*, int, size_t, usbdiag_dur_t*);
int usbdiag_burst_isoch_out(async_exch_t*, int, size_t, usbdiag_dur_t*);

int usbdiag_data_intr_in(async_exch_t*, int, size_t, usbdiag_dur_t*);
int usbdiag_data_intr_out(async_exch_t*, int, size_t, usbdiag_dur_t*);
int usbdiag_data_bulk_in(async_exch_t*, int, size_t, usbdiag_dur_t*);
int usbdiag_data_bulk_out(async_exch_t*, int, size_t, usbdiag_dur_t*);
int usbdiag_data_isoch_in(async_exch_t*, int, size_t, usbdiag_dur_t*);
int usbdiag_data_isoch_out(async_exch_t*, int, size_t, usbdiag_dur_t*);

/** USB diagnostic device communication interface. */
typedef struct {
	int (*burst_intr_in)(ddf_fun_t*, int, size_t, usbdiag_dur_t*);
	int (*burst_intr_out)(ddf_fun_t*, int, size_t, usbdiag_dur_t*);
	int (*burst_bulk_in)(ddf_fun_t*, int, size_t, usbdiag_dur_t*);
	int (*burst_bulk_out)(ddf_fun_t*, int, size_t, usbdiag_dur_t*);
	int (*burst_isoch_in)(ddf_fun_t*, int, size_t, usbdiag_dur_t*);
	int (*burst_isoch_out)(ddf_fun_t*, int, size_t, usbdiag_dur_t*);
	int (*data_intr_in)(ddf_fun_t*, int, size_t, usbdiag_dur_t*);
	int (*data_intr_out)(ddf_fun_t*, int, size_t, usbdiag_dur_t*);
	int (*data_bulk_in)(ddf_fun_t*, int, size_t, usbdiag_dur_t*);
	int (*data_bulk_out)(ddf_fun_t*, int, size_t, usbdiag_dur_t*);
	int (*data_isoch_in)(ddf_fun_t*, int, size_t, usbdiag_dur_t*);
	int (*data_isoch_out)(ddf_fun_t*, int, size_t, usbdiag_dur_t*);
} usbdiag_iface_t;

#endif
/**
 * @}
 */
