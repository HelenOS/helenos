/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdevice
 * @{
 */
/** @file
 */

#ifndef LIBDEVICE_IPC_PCI_H
#define LIBDEVICE_IPC_PCI_H

#include <ipc/common.h>

/** PCI control service requests */
typedef enum {
	PCI_GET_DEVICES = IPC_FIRST_USER_METHOD,
	PCI_DEV_GET_INFO
} pci_request_t;

#endif

/**
 * @}
 */
