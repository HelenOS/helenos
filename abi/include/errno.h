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

#define EOK             0   /* No error */
#define ENOENT         -1   /* No such entry */
#define ENOMEM         -2   /* Not enough memory */
#define ELIMIT         -3   /* Limit exceeded */
#define EREFUSED       -4   /* Connection refused */
#define EFORWARD       -5   /* Forward error */
#define EPERM          -6   /* Permission denied */

/*
 * Answerbox closed connection, call
 * sys_ipc_hangup() to close the connection.
 * Used by answerbox to close the connection.
 */
#define EHANGUP        -7

/*
 * The other party encountered an error when
 * receiving the call.
 */
#define EPARTY         -8

#define EEXISTS        -9   /* Entry already exists */
#define EBADMEM        -10  /* Bad memory pointer */
#define ENOTSUP        -11  /* Not supported */
#define EADDRNOTAVAIL  -12  /* Address not available. */
#define ETIMEOUT       -13  /* Timeout expired */
#define EINVAL         -14  /* Invalid value */
#define EBUSY          -15  /* Resource is busy */
#define EOVERFLOW      -16  /* The result does not fit its size. */
#define EINTR          -17  /* Operation was interrupted. */

#endif

/** @}
 */
