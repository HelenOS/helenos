/*
 * SPDX-FileCopyrightText: 2012 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief Texas Instruments AM/DM37x MPU on-chip interrupt controller driver.
 */

#ifndef KERN_AMDM37x_IRQC_H_
#define KERN_AMDM37x_IRQC_H_

/* AMDM37x TRM p. 1079 */
#define AMDM37x_IRC_BASE_ADDRESS     0x48200000
#define AMDM37x_IRC_SIZE             4096

#define AMDM37x_IRC_IRQ_COUNT        96
#define AMDM37x_IRC_IRQ_GROUPS_COUNT 3

#define OMAP_IRC_IRQ_COUNT        AMDM37x_IRC_IRQ_COUNT
#define OMAP_IRC_IRQ_GROUPS_COUNT AMDM37x_IRC_IRQ_GROUPS_COUNT

#include <genarch/drivers/omap/irc.h>

#endif

/**
 * @}
 */
