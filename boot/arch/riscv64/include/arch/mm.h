/*
 * SPDX-FileCopyrightText: 2017 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 */

#ifndef BOOT_riscv64_MM_H_
#define BOOT_riscv64_MM_H_

#ifndef __ASSEMBLER__
#define KA2PA(x)  (((uintptr_t) (x)) - UINT64_C(0xffff800000000000))
#define PA2KA(x)  (((uintptr_t) (x)) + UINT64_C(0xffff800000000000))
#else
#define KA2PA(x)  ((x) - 0xffff800000000000)
#define PA2KA(x)  ((x) + 0xffff800000000000)
#endif

#define PTL_DIRTY       (1 << 7)
#define PTL_ACCESSED    (1 << 6)
#define PTL_GLOBAL      (1 << 5)
#define PTL_USER        (1 << 4)
#define PTL_EXECUTABLE  (1 << 3)
#define PTL_WRITABLE    (1 << 2)
#define PTL_READABLE    (1 << 1)
#define PTL_VALID       1

#endif

/** @}
 */
