/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbehci
 * @{
 */
/** @file
 * PCI related functions needed by EHCI driver.
 */
#ifndef DRV_EHCI_PCI_H
#define DRV_EHCI_PCI_H

typedef struct hc_device hc_device_t;

extern errno_t disable_legacy(hc_device_t *);

#endif
/**
 * @}
 */
