/*
 * SPDX-FileCopyrightText: 2013 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup drvusbehci
 * @{
 */
/** @file
 * @brief EHCI driver
 */
#ifndef DRV_EHCI_HW_MEM_ACCESS_H
#define DRV_EHCI_HW_MEM_ACCESS_H

#include <byteorder.h>
#include <barrier.h>

#define EHCI_MEM32_WR(reg, val) reg = host2uint32_t_le(val)
#define EHCI_MEM32_RD(reg) uint32_t_le2host(reg)
#define EHCI_MEM32_SET(reg, val) reg |= host2uint32_t_le(val)
#define EHCI_MEM32_CLR(reg, val) reg &= host2uint32_t_le(~val)

#endif

/*
 * @}
 */
