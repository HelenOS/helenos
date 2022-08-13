/*
 * SPDX-FileCopyrightText: 2012 Maurizio Lombardi
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup kernel_genarch
 * @{
 */

/**
 * @file
 * @brief Texas Instruments AM335x MPU on-chip interrupt controller driver.
 */

#ifndef KERN_AM335x_IRQC_H_
#define KERN_AM335x_IRQC_H_

#define AM335x_IRC_BASE_ADDRESS     0x48200000
#define AM335x_IRC_SIZE             4096

#define AM335x_IRC_IRQ_COUNT        128
#define AM335x_IRC_IRQ_GROUPS_COUNT 4

#define OMAP_IRC_IRQ_COUNT        AM335x_IRC_IRQ_COUNT
#define OMAP_IRC_IRQ_GROUPS_COUNT AM335x_IRC_IRQ_GROUPS_COUNT

#include <genarch/drivers/omap/irc.h>

#endif

/**
 * @}
 */
