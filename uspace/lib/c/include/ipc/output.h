/*
 * SPDX-FileCopyrightText: 2012 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcipc
 * @{
 */
/** @file
 */

#ifndef _LIBC_IPC_OUTPUT_H_
#define _LIBC_IPC_OUTPUT_H_

#include <ipc/common.h>

typedef sysarg_t frontbuf_handle_t;

typedef enum {
	OUTPUT_YIELD = IPC_FIRST_USER_METHOD,
	OUTPUT_CLAIM,
	OUTPUT_GET_DIMENSIONS,
	OUTPUT_GET_CAPS,

	OUTPUT_FRONTBUF_CREATE,
	OUTPUT_FRONTBUF_DESTROY,

	OUTPUT_CURSOR_UPDATE,
	OUTPUT_SET_STYLE,
	OUTPUT_SET_COLOR,
	OUTPUT_SET_RGB_COLOR,

	OUTPUT_UPDATE,
	OUTPUT_DAMAGE
} output_request_t;

#endif

/**
 * @}
 */
