/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @addtogroup libnic
 * @{
 */
/**
 * @file
 * @brief Prototypes of default DDF NIC interface methods implementations
 */

#ifndef NIC_EV_H__
#define NIC_EV_H__

#include <async.h>
#include <nic/nic.h>
#include <stddef.h>

extern errno_t nic_ev_addr_changed(async_sess_t *, const nic_address_t *);
extern errno_t nic_ev_device_state(async_sess_t *, sysarg_t);
extern errno_t nic_ev_received(async_sess_t *, void *, size_t);

#endif

/** @}
 */
