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

async_sess_t *usbdiag_connect(devman_handle_t);
void usbdiag_disconnect(async_sess_t*);
int usbdiag_stress_intr_in(async_exch_t*, int, size_t);
int usbdiag_stress_intr_out(async_exch_t*, int, size_t);
int usbdiag_stress_bulk_in(async_exch_t*, int, size_t);
int usbdiag_stress_bulk_out(async_exch_t*, int, size_t);
int usbdiag_stress_isoch_in(async_exch_t*, int, size_t);
int usbdiag_stress_isoch_out(async_exch_t*, int, size_t);

/** USB diagnostic device communication interface. */
typedef struct {
	int (*stress_intr_in)(ddf_fun_t*, int, size_t);
	int (*stress_intr_out)(ddf_fun_t*, int, size_t);
	int (*stress_bulk_in)(ddf_fun_t*, int, size_t);
	int (*stress_bulk_out)(ddf_fun_t*, int, size_t);
	int (*stress_isoch_in)(ddf_fun_t*, int, size_t);
	int (*stress_isoch_out)(ddf_fun_t*, int, size_t);
} usbdiag_iface_t;

#endif
/**
 * @}
 */
