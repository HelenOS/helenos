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
#ifndef DRV_EHCI_HW_STRUCT_FSTN_H
#define DRV_EHCI_HW_STRUCT_FSTN_H

#include "link_pointer.h"

typedef struct fstn {
	link_pointer_t normal_path;
	link_pointer_t back_path;
} fstn_t;

#endif
/**
 * @}
 */
