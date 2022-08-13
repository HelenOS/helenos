/*
 * SPDX-FileCopyrightText: 2012 Julia Medvedeva
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup udf
 * @{
 */

#ifndef UDF_CKSUM_H_
#define UDF_CKSUM_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define UDF_TAG_SIZE  16

extern uint16_t udf_unicode_cksum(uint16_t *, size_t);
extern uint8_t udf_tag_checksum(uint8_t *);

#endif /* UDF_CKSUM_H_ */

/**
 * @}
 */
