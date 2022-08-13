/*
 * SPDX-FileCopyrightText: 2012 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup audio
 * @brief HelenOS sound server
 * @{
 */
/** @file
 */

#ifndef PCM_FORMAT_H_
#define PCM_FORMAT_H_

#include <assert.h>
#include <stdbool.h>
#include <pcm/sample_format.h>

/** Linear PCM audio parameters */
typedef struct {
	unsigned channels;
	unsigned sampling_rate;
	pcm_sample_format_t sample_format;
} pcm_format_t;

extern const pcm_format_t AUDIO_FORMAT_DEFAULT;
extern const pcm_format_t AUDIO_FORMAT_ANY;

/**
 * Frame size helper function.
 * @param a pointer to a PCM format structure.
 * @return Size in bytes.
 */
static inline size_t pcm_format_frame_size(const pcm_format_t *a)
{
	return pcm_sample_format_frame_size(a->channels, a->sample_format);
}

/**
 * Convert byte size to frame count.
 * @param size Byte-size.
 * @param a pointer to a PCM format structure.
 * @return Frame count.
 */
static inline size_t pcm_format_size_to_frames(size_t size,
    const pcm_format_t *a)
{
	return pcm_sample_format_size_to_frames(size, a->channels,
	    a->sample_format);
}

/**
 * Convert byte size to audio playback time.
 * @param size Byte-size.
 * @param a pointer to a PCM format structure.
 * @return Number of microseconds.
 */
static inline usec_t pcm_format_size_to_usec(size_t size, const pcm_format_t *a)
{
	return pcm_sample_format_size_to_usec(size, a->sampling_rate,
	    a->channels, a->sample_format);
}

bool pcm_format_same(const pcm_format_t *a, const pcm_format_t *b);

/**
 * Helper function, compares with ANY metaformat.
 * @param f pointer to format structure.
 * @return True if @p f points to ANY format, false otherwise.
 */
static inline bool pcm_format_is_any(const pcm_format_t *f)
{
	return pcm_format_same(f, &AUDIO_FORMAT_ANY);
}
void pcm_format_silence(void *dst, size_t size, const pcm_format_t *f);
errno_t pcm_format_convert_and_mix(void *dst, size_t dst_size, const void *src,
    size_t src_size, const pcm_format_t *sf, const pcm_format_t *df);
errno_t pcm_format_mix(void *dst, const void *src, size_t size, const pcm_format_t *f);
errno_t pcm_format_convert(pcm_format_t a, void *srca, size_t sizea,
    pcm_format_t b, void *srcb, size_t *sizeb);

#endif

/**
 * @}
 */
