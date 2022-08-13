/*
 * SPDX-FileCopyrightText: 2012 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbohci
 * @{
 */
/** @file
 * @brief OHCI driver
 */

#ifndef DRV_OHCI_HW_MEM_ACCESS_H
#define DRV_OHCI_HW_MEM_ACCESS_H

#include <byteorder.h>

#define OHCI_MEM32_WR(reg, val) reg = host2uint32_t_le(val)
#define OHCI_MEM32_RD(reg) uint32_t_le2host(reg)
#define OHCI_MEM32_SET(reg, val) reg |= host2uint32_t_le(val)
#define OHCI_MEM32_CLR(reg, val) reg &= host2uint32_t_le(~val)

#endif

/*
 * @}
 */
