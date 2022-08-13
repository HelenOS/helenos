/*
 * SPDX-FileCopyrightText: 1995 Micro Design International, Inc.
 * SPDX-FileCopyrightText: 2012 Julia Medvedeva
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * OSTA compliant Unicode compression, uncompression routines,
 * file name translation routine for OS/2, Windows 95, Windows NT,
 * Macintosh and UNIX.
 * Written by Jason M. Rinn.
 * Micro Design International gives permission for the free use of the
 * following source code.
 */

#ifndef UDF_OSTA_H_
#define UDF_OSTA_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "udf_types.h"

#define MAXLEN    255
#define MAX_BUF   1024
#define EXT_SIZE  5

#define ILLEGAL_CHAR_MARK  0x005F
#define CRC_MARK           0x0023
#define PERIOD             0x002E

extern size_t udf_translate_name(uint16_t *, uint16_t *, size_t);
extern void udf_to_unix_name(char *, size_t, char *, size_t, udf_charspec_t *);

#endif /* UDF_OSTA_H_ */
