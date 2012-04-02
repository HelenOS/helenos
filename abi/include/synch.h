/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup sync
 * @{
 */
/** @file
 */

#ifndef ABI_SYNCH_H_
#define ABI_SYNCH_H_

/** Request with no timeout. */
#define SYNCH_NO_TIMEOUT  0

/** No flags specified. */
#define SYNCH_FLAGS_NONE           0
/** Non-blocking operation request. */
#define SYNCH_FLAGS_NON_BLOCKING   (1 << 0)
/** Interruptible operation. */
#define SYNCH_FLAGS_INTERRUPTIBLE  (1 << 1)

/** Could not satisfy the request without going to sleep. */
#define ESYNCH_WOULD_BLOCK  1
/** Timeout occurred. */
#define ESYNCH_TIMEOUT      2
/** Sleep was interrupted. */
#define ESYNCH_INTERRUPTED  4
/** Operation succeeded without sleeping. */
#define ESYNCH_OK_ATOMIC    8
/** Operation succeeded and did sleep. */
#define ESYNCH_OK_BLOCKED   16

#define SYNCH_FAILED(rc) \
	((rc) & (ESYNCH_WOULD_BLOCK | ESYNCH_TIMEOUT | ESYNCH_INTERRUPTED))

#define SYNCH_OK(rc) \
	((rc) & (ESYNCH_OK_ATOMIC | ESYNCH_OK_BLOCKED))

#endif

/** @}
 */
