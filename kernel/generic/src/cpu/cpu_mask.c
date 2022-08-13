/*
 * SPDX-FileCopyrightText: 2012 Adam Hraska
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */

/**
 * @file
 * @brief CPU mask manipulation functions.
 */
#include <assert.h>
#include <cpu/cpu_mask.h>
#include <cpu.h>
#include <config.h>

static const size_t word_size = sizeof(unsigned int);
static const size_t word_bit_cnt = 8 * sizeof(unsigned int);

/** Returns the size of cpu_mask_t for the detected number of cpus in bytes. */
size_t cpu_mask_size(void)
{
	size_t word_cnt = (config.cpu_count + word_bit_cnt - 1) / word_bit_cnt;
	return word_cnt * word_size;
}

/** Add first cpu_cnt cpus to the mask, ie sets the first cpu_cnt bits. */
static void cpu_mask_count(cpu_mask_t *cpus, size_t cpu_cnt)
{
	assert(NULL != cpus);
	assert(cpu_cnt <= config.cpu_count);

	for (size_t active_word = 0;
	    (active_word + 1) * word_bit_cnt <= cpu_cnt;
	    ++active_word) {
		/* Set all bits in the cell/word. */
		cpus->mask[active_word] = -1;
	}

	size_t remaining_bits = (cpu_cnt % word_bit_cnt);
	if (0 < remaining_bits) {
		/* Set lower remaining_bits of the last word. */
		cpus->mask[cpu_cnt / word_bit_cnt] = (1 << remaining_bits) - 1;
	}
}

/** Sets bits corresponding to the active cpus, ie the first
 * config.cpu_active cpus.
 */
void cpu_mask_active(cpu_mask_t *cpus)
{
	cpu_mask_none(cpus);
	cpu_mask_count(cpus, config.cpu_active);
}

/** Sets bits for all cpus of the mask. */
void cpu_mask_all(cpu_mask_t *cpus)
{
	cpu_mask_count(cpus, config.cpu_count);
}

/** Resets/removes all bits. */
void cpu_mask_none(cpu_mask_t *cpus)
{
	assert(cpus);

	size_t word_cnt = cpu_mask_size() / word_size;

	for (size_t word = 0; word < word_cnt; ++word) {
		cpus->mask[word] = 0;
	}
}

/** Sets the bit corresponding to cpu_id to true. */
void cpu_mask_set(cpu_mask_t *cpus, unsigned int cpu_id)
{
	size_t word = cpu_id / word_bit_cnt;
	size_t word_pos = cpu_id % word_bit_cnt;

	cpus->mask[word] |= (1U << word_pos);
}

/** Resets the bit corresponding to cpu_id to false. */
void cpu_mask_reset(cpu_mask_t *cpus, unsigned int cpu_id)
{
	size_t word = cpu_id / word_bit_cnt;
	size_t word_pos = cpu_id % word_bit_cnt;

	cpus->mask[word] &= ~(1U << word_pos);
}

/** Returns true if the bit corresponding to cpu_id is set. */
bool cpu_mask_is_set(cpu_mask_t *cpus, unsigned int cpu_id)
{
	size_t word = cpu_id / word_bit_cnt;
	size_t word_pos = cpu_id % word_bit_cnt;

	return 0 != (cpus->mask[word] & (1U << word_pos));
}

/** Returns true if no bits are set. */
bool cpu_mask_is_none(cpu_mask_t *cpus)
{
	size_t word_cnt = cpu_mask_size() / word_size;

	for (size_t word = 0; word < word_cnt; ++word) {
		if (cpus->mask[word])
			return false;
	}

	return true;
}

/** @}
 */
