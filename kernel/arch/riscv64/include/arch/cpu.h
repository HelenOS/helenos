/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_riscv64
 * @{
 */
/** @file
 */

#ifndef KERN_riscv64_CPU_H_
#define KERN_riscv64_CPU_H_

#define SSTATUS_SIE_MASK  0x00000002

#define SATP_PFN_MASK  UINT64_C(0x00000fffffffffff)

#define SATP_MODE_MASK  UINT64_C(0xf000000000000000)
#define SATP_MODE_BARE  UINT64_C(0x0000000000000000)
#define SATP_MODE_SV39  UINT64_C(0x8000000000000000)
#define SATP_MODE_SV48  UINT64_C(0x9000000000000000)

#ifndef __ASSEMBLER__

typedef struct {
} cpu_arch_t;

extern void cpu_setup_fpu(void);

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
