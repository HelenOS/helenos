/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BOOT_riscv64_UCB_H_
#define BOOT_riscv64_UCB_H_

/** University of California, Berkeley extensions to RISC-V
 *
 * Non-standard host-target interface extension targeting
 * primarily the Spike ISA Simulator.
 */

#include <stddef.h>
#include <stdint.h>

extern volatile uint64_t tohost;
extern volatile uint64_t fromhost;

#define HTIF_DEVICE_CONSOLE  1

#define HTIF_CONSOLE_PUTC  1

extern void htif_cmd(uint8_t device, uint8_t cmd, uint64_t payload);

#endif
