/*
 * Copyright (c) 2011 Martin Decky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup cdfs
 * @{
 */

#ifndef CDFS_CDFS_ENDIAN_H_
#define CDFS_CDFS_ENDIAN_H_

#include <stdint.h>

#if !(defined(__BE__) ^ defined(__LE__))
#error The architecture must be either big-endian or little-endian.
#endif

typedef struct {
	uint16_t le;
	uint16_t be;
} __attribute__((packed)) uint16_t_lb;

typedef struct {
	uint32_t le;
	uint32_t be;
} __attribute__((packed)) uint32_t_lb;

static inline uint16_t uint16_lb(uint16_t_lb val)
{
#ifdef __LE__
	return val.le;
#endif

#ifdef __BE__
	return val.be;
#endif
}

static inline uint32_t uint32_lb(uint32_t_lb val)
{
#ifdef __LE__
	return val.le;
#endif

#ifdef __BE__
	return val.be;
#endif
}

#endif

/**
 * @}
 */
