/*
 * SPDX-FileCopyrightText: 2017 Petr Manek
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
errno_t usbdiag_test_out(async_exch_t *,
    const usbdiag_test_params_t *, usbdiag_test_results_t *);

/** USB diagnostic device communication interface. */
typedef struct {
	errno_t (*test_in)(ddf_fun_t *,
	    const usbdiag_test_params_t *, usbdiag_test_results_t *);
	errno_t (*test_out)(ddf_fun_t *,
	    const usbdiag_test_params_t *, usbdiag_test_results_t *);
} usbdiag_iface_t;

#endif
/**
 * @}
 */
