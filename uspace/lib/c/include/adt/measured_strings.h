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

/** @addtogroup net
 *  @{
 */

/** @file
 *  Character string with measured length.
 *  The structure has been designed for serialization of character strings between modules.
 */

#ifndef __MEASURED_STRINGS_H__
#define __MEASURED_STRINGS_H__

#include <sys/types.h>

/** Type definition of the character string with measured length.
 *  @see measured_string
 */
typedef struct measured_string	measured_string_t;

/** Type definition of the character string with measured length pointer.
 *  @see measured_string
 */
typedef measured_string_t *		measured_string_ref;

/** Character string with measured length.
 *  This structure has been designed for serialization of character strings between modules.
 */
struct	measured_string{
	/** Character string data.
	 */
	char * value;
	/** Character string length.
	 */
	size_t length;
};

/** Creates a&nbsp;new measured string bundled with a&nbsp;copy of the given string itself as one memory block.
 *  If the measured string is being freed, whole memory block is freed.
 *  The measured string should be used only as a&nbsp;constant.
 *  @param[in] string The initial character string to be stored.
 *  @param[in] length The length of the given string without the terminating zero ('/0') character. If the length is zero (0), the actual length is computed. The given length is used and appended with the terminating zero ('\\0') character otherwise.
 *  @returns The new bundled character string with measured length.
 *  @returns NULL if there is not enough memory left.
 */
extern measured_string_ref measured_string_create_bulk(const char * string, size_t length);

/** Copies the given measured string with separated header and data parts.
 *  @param[in] source The source measured string to be copied.
 *  @returns The copy of the given measured string.
 *  @returns NULL if the source parameter is NULL.
 *  @returns NULL if there is not enough memory left.
 */
extern measured_string_ref measured_string_copy(measured_string_ref source);

/** Receives a&nbsp;measured strings array from a&nbsp;calling module.
 *  Creates the array and the data memory blocks.
 *  This method should be used only while processing IPC messages as the array size has to be negotiated in advance.
 *  @param[out] strings The received measured strings array.
 *  @param[out] data The measured strings data. This memory block stores the actual character strings.
 *  @param[in] count The size of the measured strings array.
 *  @returns EOK on success.
 *  @returns EINVAL if the strings or data parameter is NULL.
 *  @returns EINVAL if the count parameter is zero (0).
 *  @returns EINVAL if the sent array differs in size.
 *  @returns EINVAL if there is inconsistency in sent measured strings' lengths (should not occur).
 *  @returns ENOMEM if there is not enough memory left.
 *  @returns Other error codes as defined for the async_data_write_finalize() function.
 */
extern int measured_strings_receive(measured_string_ref * strings, char ** data, size_t count);

/** Replies the given measured strings array to a&nbsp;calling module.
 *  This method should be used only while processing IPC messages as the array size has to be negotiated in advance.
 *  @param[in] strings The measured strings array to be transferred.
 *  @param[in] count The measured strings array size.
 *  @returns EOK on success.
 *  @returns EINVAL if the strings parameter is NULL.
 *  @returns EINVAL if the count parameter is zero (0).
 *  @returns EINVAL if the calling module does not accept the given array size.
 *  @returns EINVAL if there is inconsistency in sent measured strings' lengths (should not occur).
 *  @returns Other error codes as defined for the async_data_read_finalize() function.
 */
extern int measured_strings_reply(const measured_string_ref strings, size_t count);

/** Receives a&nbsp;measured strings array from another module.
 *  Creates the array and the data memory blocks.
 *  This method should be used only following other IPC messages as the array size has to be negotiated in advance.
 *  @param[in] phone The other module phone.
 *  @param[out] strings The returned measured strings array.
 *  @param[out] data The measured strings data. This memory block stores the actual character strings.
 *  @param[in] count The size of the measured strings array.
 *  @returns EOK on success.
 *  @returns EINVAL if the strings or data parameter is NULL.
 *  @returns EINVAL if the phone or count parameter is not positive (<=0).
 *  @returns EINVAL if the sent array differs in size.
 *  @returns ENOMEM if there is not enough memory left.
 *  @returns Other error codes as defined for the async_data_read_start() function.
 */
extern int measured_strings_return(int phone, measured_string_ref * strings, char ** data, size_t count);

/** Sends the given measured strings array to another module.
 *  This method should be used only following other IPC messages as the array size has to be negotiated in advance.
 *  @param[in] phone The other module phone.
 *  @param[in] strings The measured strings array to be transferred.
 *  @param[in] count The measured strings array size.
 *  @returns EOK on success.
 *  @returns EINVAL if the strings parameter is NULL.
 *  @returns EINVAL if the phone or count parameter is not positive (<=0).
 *  @returns Other error codes as defined for the async_data_write_start() function.
 */
extern int measured_strings_send(int phone, const measured_string_ref strings, size_t count);

#endif

/** @}
 */

