/*
 * SPDX-FileCopyrightText: 2018 Jiří Zárevúcky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BOOT_PAYLOAD_H_
#define BOOT_PAYLOAD_H_

#include <arch/types.h>
#include <stddef.h>
#include <stdint.h>

extern uint8_t payload_start[];
extern uint8_t payload_end[];

extern uint8_t loader_start[];
extern uint8_t loader_end[];

size_t payload_unpacked_size(void);
void extract_payload(taskmap_t *, uint8_t *, uint8_t *, uintptr_t,
    void (*)(void *, size_t));

#endif
