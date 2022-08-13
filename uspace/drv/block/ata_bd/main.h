/*
 * SPDX-FileCopyrightText: 2013 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup ata_bd
 * @{
 */
/** @file ATA driver main module
 */

#ifndef __ATA_MAIN_H__
#define __ATA_MAIN_H__

#include "ata_bd.h"

extern errno_t ata_fun_create(disk_t *);
extern errno_t ata_fun_remove(disk_t *);
extern errno_t ata_fun_unbind(disk_t *);

#endif

/** @}
 */
