/*
 * SPDX-FileCopyrightText: 2017 Petr Manek
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbdiag
 * @{
 */
/** @file
 * USB diagnostic tests.
 */

#ifndef USBDIAG_TESTS_H_
#define USBDIAG_TESTS_H_

#include <ddf/driver.h>

errno_t usbdiag_dev_test_in(ddf_fun_t *, const usbdiag_test_params_t *,
    usbdiag_test_results_t *);
errno_t usbdiag_dev_test_out(ddf_fun_t *, const usbdiag_test_params_t *,
    usbdiag_test_results_t *);

#endif /* USBDIAG_TESTS_H_ */

/**
 * @}
 */
