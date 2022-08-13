/*
 * SPDX-FileCopyrightText: 2017 Petr Manek
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup tmon
 * @{
 */
/**
 * @file Testing framework definitions.
 */

#ifndef TMON_TF_H_
#define TMON_TF_H_

#include <async.h>
#include <usbdiag_iface.h>

/** Operations to implement by all tests. */
typedef struct tmon_test_ops {
	errno_t (*pre_run)(void *);
	errno_t (*run)(async_exch_t *, const void *);
	errno_t (*read_params)(int, char **, void **);
} tmon_test_ops_t;

int tmon_test_main(int, char **, const tmon_test_ops_t *);

char *tmon_format_size(double, const char *);
char *tmon_format_duration(usbdiag_dur_t, const char *);

#endif /* TMON_TF_H_ */

/** @}
 */
