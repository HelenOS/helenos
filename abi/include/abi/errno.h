/*
 * Copyright (c) 2005 Martin Decky
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

/** @addtogroup generic
 * @{
 */
/** @file
 */

#ifndef ABI_ERRNO_H_
#define ABI_ERRNO_H_

/**
 * Values in the range [-1, -255] are kernel error codes,
 * values in the range [-256, -512] are user error codes.
 */

typedef enum {
	EOK           =  0,   /* No error */
	ENOENT        = -1,   /* No such entry */
	ENOMEM        = -2,   /* Not enough memory */
	ELIMIT        = -3,   /* Limit exceeded */
	EREFUSED      = -4,   /* Connection refused */
	EFORWARD      = -5,   /* Forward error */
	EPERM         = -6,   /* Permission denied */

/*
 * Answerbox closed connection, call
 * sys_ipc_hangup() to close the connection.
 * Used by answerbox to close the connection.
 */
	EHANGUP       = -7,

/*
 * The other party encountered an error when
 * receiving the call.
 */
	EPARTY        = -8,

	EEXIST        = -9,   /* Entry already exists */
	EBADMEM       = -10,  /* Bad memory pointer */
	ENOTSUP       = -11,  /* Not supported */
	EADDRNOTAVAIL = -12,  /* Address not available. */
	ETIMEOUT      = -13,  /* Timeout expired */
	EINVAL        = -14,  /* Invalid value */
	EBUSY         = -15,  /* Resource is busy */
	EOVERFLOW     = -16,  /* The result does not fit its size. */
	EINTR         = -17,  /* Operation was interrupted. */

	EMFILE        = -18,
	ENAMETOOLONG  = -256,
	EISDIR        = -257,
	ENOTDIR       = -258,
	ENOSPC        = -259,
	ENOTEMPTY     = -261,
	EBADF         = -262,
	EDOM          = -263,
	ERANGE        = -264,
	EXDEV         = -265,
	EIO           = -266,
	EMLINK        = -267,
	ENXIO         = -268,
	ENOFS         = -269,

/** Bad checksum. */
	EBADCHECKSUM  = -300,

/** USB: stalled operation. */
	ESTALL        = -301,

/** Empty resource (no data). */
	EEMPTY        = -302,

/** Negative acknowledgment. */
	ENAK          = -303,

/** The requested operation was not performed. Try again later. */
	EAGAIN        = -11002,
} errno_t;


#endif

/** @}
 */
