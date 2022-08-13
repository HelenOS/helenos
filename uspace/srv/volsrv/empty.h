/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup volsrv
 * @{
 */
/**
 * @file
 * @brief
 */

#ifndef EMPTY_H_
#define EMPTY_H_

#include <loc.h>

extern errno_t volsrv_part_is_empty(service_id_t, bool *);
extern errno_t volsrv_part_empty(service_id_t);

#endif

/** @}
 */
