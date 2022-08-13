/*
 * SPDX-FileCopyrightText: 2012 Frantisek Princ
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libext4
 * @{
 */

#ifndef LIBEXT4_BITMAP_H_
#define LIBEXT4_BITMAP_H_

#include <stdint.h>

extern void ext4_bitmap_free_bit(uint8_t *, uint32_t);
extern void ext4_bitmap_free_bits(uint8_t *, uint32_t, uint32_t);
extern void ext4_bitmap_set_bit(uint8_t *, uint32_t);
extern bool ext4_bitmap_is_free_bit(uint8_t *, uint32_t);
extern errno_t ext4_bitmap_find_free_byte_and_set_bit(uint8_t *, uint32_t,
    uint32_t *, uint32_t);
extern errno_t ext4_bitmap_find_free_bit_and_set(uint8_t *, uint32_t, uint32_t *,
    uint32_t);

#endif

/**
 * @}
 */
