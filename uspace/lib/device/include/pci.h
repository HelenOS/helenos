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

#ifndef LIBDEVICE_PCI_H
#define LIBDEVICE_PCI_H

#include <errno.h>
#include <ipc/devman.h>
#include <loc.h>
#include <types/pci.h>

extern errno_t pci_open(service_id_t, pci_t **);
extern void pci_close(pci_t *);
extern errno_t pci_get_devices(pci_t *, devman_handle_t **, size_t *);
extern errno_t pci_dev_get_info(pci_t *, devman_handle_t, pci_dev_info_t *);

#endif

/** @}
 */
