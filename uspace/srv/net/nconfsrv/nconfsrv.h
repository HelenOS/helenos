/*
 * SPDX-FileCopyrightText: 2013 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup inetsrv
 * @{
 */
/**
 * @file
 * @brief
 */

#ifndef INETSRV_H_
#define INETSRV_H_

#include <adt/list.h>
#include <ipc/loc.h>

typedef struct {
	link_t link_list;
	service_id_t svc_id;
	char *svc_name;
} ncs_link_t;

#endif

/** @}
 */
