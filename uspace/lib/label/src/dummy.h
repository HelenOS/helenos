/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup liblabel
 * @{
 */
/**
 * @file Dummy label (fallback for disks that have no recognized label).
 */

#ifndef LIBLABEL_DUMMY_H_
#define LIBLABEL_DUMMY_H_

#include <types/liblabel.h>

extern label_ops_t dummy_label_ops;

#endif

/** @}
 */
