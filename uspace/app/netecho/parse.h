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
 *  Generic command line arguments parsing functions.
 */

#ifndef __NET_APP_PARSE__
#define __NET_APP_PARSE__

#include <socket.h>

/** Translates the character string to the address family number.
 *  @param[in] name The address family name.
 *  @returns The corresponding address family number.
 *  @returns EAFNOSUPPORTED if the address family is not supported.
 */
extern int parse_address_family(const char * name);

/** Parses the next parameter as an integral number.
 *  The actual parameter is pointed by the index.
 *  Parses the offseted actual parameter value if the offset is set or the next one if not.
 *  @param[in] argc The total number of the parameters.
 *  @param[in] argv The parameters.
 *  @param[in,out] index The actual parameter index. The index is incremented by the number of processed parameters.
 *  @param[out] value The parsed parameter value.
 *  @param[in] name The parameter name to be printed on errors.
 *  @param[in] offset The value offset in the actual parameter. If not set, the next parameter is parsed instead.
 *  @returns EOK on success.
 *  @returns EINVAL if the parameter is missing.
 *  @returns EINVAL if the parameter is in wrong format.
 */
extern int parse_parameter_int(int argc, char ** argv, int * index, int * value, const char * name, int offset);

/** Parses the next named parameter as an integral number.
 *  The actual parameter is pointed by the index.
 *  Uses the offseted actual parameter if the offset is set or the next one if not.
 *  Translates the parameter using the parse_value function.
 *  Increments the actual index by the number of processed parameters.
 *  @param[in] argc The total number of the parameters.
 *  @param[in] argv The parameters.
 *  @param[in,out] index The actual parameter index. The index is incremented by the number of processed parameters.
 *  @param[out] value The parsed parameter value.
 *  @param[in] name The parameter name to be printed on errors.
 *  @param[in] offset The value offset in the actual parameter. If not set, the next parameter is parsed instead.
 *  @param[in] parse_value The translation function to parse the named value.
 *  @returns EOK on success.
 *  @returns EINVAL if the parameter is missing.
 *  @returns ENOENT if the parameter name has not been found.
 */
extern int parse_parameter_name_int(int argc, char ** argv, int * index, int * value, const char * name, int offset, int (*parse_value)(const char * value));

/** Parses the next parameter as a character string.
 *  The actual parameter is pointed by the index.
 *  Uses the offseted actual parameter value if the offset is set or the next one if not.
 *  Increments the actual index by the number of processed parameters.
 *  @param[in] argc The total number of the parameters.
 *  @param[in] argv The parameters.
 *  @param[in,out] index The actual parameter index. The index is incremented by the number of processed parameters.
 *  @param[out] value The parsed parameter value.
 *  @param[in] name The parameter name to be printed on errors.
 *  @param[in] offset The value offset in the actual parameter. If not set, the next parameter is parsed instead.
 *  @returns EOK on success.
 *  @returns EINVAL if the parameter is missing.
 */
extern int parse_parameter_string(int argc, char ** argv, int * index, char ** value, const char * name, int offset);

/** Translates the character string to the protocol family number.
 *  @param[in] name The protocol family name.
 *  @returns The corresponding protocol family number.
 *  @returns EPFNOSUPPORTED if the protocol family is not supported.
 */
extern int parse_protocol_family(const char * name);

/** Translates the character string to the socket type number.
 *  @param[in] name The socket type name.
 *  @returns The corresponding socket type number.
 *  @returns ESOCKNOSUPPORTED if the socket type is not supported.
 */
extern int parse_socket_type(const char * name);

/** Prints the parameter unrecognized message and the application help.
 *  @param[in] index The index of the parameter.
 *  @param[in] parameter The parameter name.
 */
extern void print_unrecognized(int index, const char * parameter);

#endif

/** @}
 */
