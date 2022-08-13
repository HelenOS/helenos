/*
 * SPDX-FileCopyrightText: 2014 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup edit
 * @{
 */
/**
 * @file
 */

#ifndef TASKDUMP_H
#define TASKDUMP_H

#include <stdint.h>

extern errno_t td_stacktrace(uintptr_t, uintptr_t);

#endif

/** @}
 */
