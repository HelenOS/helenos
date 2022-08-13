/*
 * SPDX-FileCopyrightText: 2017 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup abi_generic
 * @{
 */
/** @file
 */

#ifndef _ABI_CAP_H_
#define _ABI_CAP_H_

#include <stdbool.h>
#include <stdint.h>

typedef void *cap_handle_t;

typedef struct {
} *cap_call_handle_t;

typedef struct {
} *cap_phone_handle_t;

typedef struct {
} *cap_irq_handle_t;

typedef struct {
} *cap_waitq_handle_t;

static cap_handle_t const CAP_NIL = 0;

static inline bool cap_handle_valid(cap_handle_t handle)
{
	return handle != CAP_NIL;
}

static inline intptr_t cap_handle_raw(cap_handle_t handle)
{
	return (intptr_t) handle;
}

#endif

/** @}
 */
