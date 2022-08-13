/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup pciintel
 * @{
 */
/** @file
 */

#ifndef CTL_H_
#define CTL_H_

#include <async.h>

extern void pci_ctl_connection(ipc_call_t *icall, void *arg);

#endif

/**
 * @}
 */
