/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdevice
 * @{
 */
/** @file
 */

#ifndef LIBDEVICE_IO_LABEL_H
#define LIBDEVICE_IO_LABEL_H

#include <types/label.h>

extern int label_type_format(label_type_t, char **);
extern int label_pkind_format(label_pkind_t, char **);

#endif

/** @}
 */
