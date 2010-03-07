/*
 * Copyright (c) 2009 Lukas Mejdrech
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

/** @addtogroup net_app
 *  @{
 */

/** @file
 *  Generic application error printing functions.
 */

#ifndef __NET_APP_PRINT__
#define __NET_APP_PRINT__

/** Returns whether the error code may be an ICMP error code.
 *  @param[in] error_code The error code.
 *  @returns A value indicating whether the error code may be an ICMP error code.
 */
#define IS_ICMP_ERROR(error_code)		((error_code) > 0)

/** Returns whether the error code may be socket error code.
 *  @param[in] error_code The error code.
 *  @returns A value indicating whether the error code may be a socket error code.
 */
#define IS_SOCKET_ERROR(error_code)	((error_code) < 0)

/** Prints the error description.
 *  Supports ICMP and socket error codes.
 *  @param[in] output The description output stream. May be NULL.
 *  @param[in] error_code The error code.
 *  @param[in] prefix The error description prefix. May be NULL.
 *  @param[in] suffix The error description suffix. May be NULL.
 */
void print_error(FILE * output, int error_code, const char * prefix, const char * suffix);

/** Prints the specific ICMP error description.
 *  @param[in] output The description output stream. May be NULL.
 *  @param[in] error_code The ICMP error code.
 *  @param[in] prefix The error description prefix. May be NULL.
 *  @param[in] suffix The error description suffix. May be NULL.
 */
void icmp_print_error(FILE * output, int error_code, const char * prefix, const char * suffix);

/** Prints the specific socket error description.
 *  @param[in] output The description output stream. May be NULL.
 *  @param[in] error_code The socket error code.
 *  @param[in] prefix The error description prefix. May be NULL.
 *  @param[in] suffix The error description suffix. May be NULL.
 */
void socket_print_error(FILE * output, int error_code, const char * prefix, const char * suffix);

#endif

/** @}
 */
