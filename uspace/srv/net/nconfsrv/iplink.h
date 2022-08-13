/*
 * SPDX-FileCopyrightText: 2013 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup nconfsrv
 * @{
 */
/**
 * @file
 * @brief
 */

#ifndef NCONFSRV_IPLINK_H_
#define NCONFSRV_IPLINK_H_

#include <stddef.h>
#include "nconfsrv.h"

extern errno_t ncs_link_discovery_start(void);
extern ncs_link_t *ncs_link_get_by_id(sysarg_t);
extern errno_t ncs_link_get_id_list(sysarg_t **, size_t *);

#endif

/** @}
 */
