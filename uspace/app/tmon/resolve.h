/*
 * SPDX-FileCopyrightText: 2017 Petr Manek
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup tmon
 * @{
 */
/**
 * @file USB diagnostic device resolving.
 */

#ifndef TMON_RESOLVE_H_
#define TMON_RESOLVE_H_

#include <devman.h>

errno_t tmon_resolve_default(devman_handle_t *);
errno_t tmon_resolve_named(const char *, devman_handle_t *);

#endif /* TMON_RESOLVE_H_ */

/** @}
 */
