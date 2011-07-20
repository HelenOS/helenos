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

/** @addtogroup libc
 *  @{
 */

/** @file
 * Character string with measured length implementation.
 * @see measured_strings.h
 */

#include <adt/measured_strings.h>
#include <malloc.h>
#include <mem.h>
#include <unistd.h>
#include <errno.h>
#include <async.h>

/** Creates a new measured string bundled with a copy of the given string
 * itself as one memory block.
 *
 * If the measured string is being freed, whole memory block is freed.
 * The measured string should be used only as a constant.
 *
 * @param[in] string	The initial character string to be stored.
 * @param[in] length	The length of the given string without the terminating
 *			zero ('\0') character. If the length is zero, the actual
 *			length is computed. The given length is used and
 *			appended with the terminating zero ('\0') character
 *			otherwise.
 * @return		The new bundled character string with measured length.
 * @return		NULL if there is not enough memory left.
 */
measured_string_t *
measured_string_create_bulk(const uint8_t *string, size_t length)
{
	measured_string_t *new;

	if (length == 0) {
		while (string[length])
			length++;
	}
	new = (measured_string_t *) malloc(sizeof(measured_string_t) +
	    (sizeof(uint8_t) * (length + 1)));
	if (!new)
		return NULL;

	new->length = length;
	new->value = ((uint8_t *) new) + sizeof(measured_string_t);
	/* Append terminating zero explicitly - to be safe */
	memcpy(new->value, string, new->length);
	new->value[new->length] = '\0';

	return new;
}

/** Copies the given measured string with separated header and data parts.
 *
 * @param[in] source	The source measured string to be copied.
 * @return		The copy of the given measured string.
 * @return		NULL if the source parameter is NULL.
 * @return		NULL if there is not enough memory left.
 */
measured_string_t *measured_string_copy(measured_string_t *source)
{
	measured_string_t *new;

	if (!source)
		return NULL;

	new = (measured_string_t *) malloc(sizeof(measured_string_t));
	if (new) {
		new->value = (uint8_t *) malloc(source->length + 1);
		if (new->value) {
			new->length = source->length;
			memcpy(new->value, source->value, new->length);
			new->value[new->length] = '\0';
			return new;
		}
		free(new);
	}

	return NULL;
}

/** Receives a measured strings array from a calling module.
 *
 * Creates the array and the data memory blocks.
 * This method should be used only while processing IPC messages as the array
 * size has to be negotiated in advance.
 *
 *  @param[out] strings	The received measured strings array.
 *  @param[out] data	The measured strings data. This memory block stores the
 *			actual character strings.
 *  @param[in] count	The size of the measured strings array.
 *  @return		EOK on success.
 *  @return		EINVAL if the strings or data parameter is NULL.
 *  @return		EINVAL if the count parameter is zero (0).
 *  @return		EINVAL if the sent array differs in size.
 *  @return		EINVAL if there is inconsistency in sent measured
 *			strings' lengths (should not occur).
 *  @return		ENOMEM if there is not enough memory left.
 *  @return		Other error codes as defined for the
 *			async_data_write_finalize() function.
 */
int
measured_strings_receive(measured_string_t **strings, uint8_t **data,
    size_t count)
{
	size_t *lengths;
	size_t index;
	size_t length;
	uint8_t *next;
	ipc_callid_t callid;
	int rc;

	if ((!strings) || (!data) || (count <= 0))
		return EINVAL;

	lengths = (size_t *) malloc(sizeof(size_t) * (count + 1));
	if (!lengths)
		return ENOMEM;

	if ((!async_data_write_receive(&callid, &length)) ||
	    (length != sizeof(size_t) * (count + 1))) {
		free(lengths);
		return EINVAL;
	}
	rc = async_data_write_finalize(callid, lengths, length);
	if (rc != EOK) {
		free(lengths);
		return rc;
	}

	*data = malloc(lengths[count]);
	if (!*data) {
		free(lengths);
		return ENOMEM;
	}
	(*data)[lengths[count] - 1] = '\0';

	*strings = (measured_string_t *) malloc(sizeof(measured_string_t) *
	    count);
	if (!*strings) {
		free(lengths);
		free(*data);
		return ENOMEM;
	}

	next = *data;
	for (index = 0; index < count; index++) {
		(*strings)[index].length = lengths[index];
		if (lengths[index] > 0) {
			if (!async_data_write_receive(&callid, &length) ||
			    (length != lengths[index])) {
				free(*data);
				free(*strings);
				free(lengths);
				return EINVAL;
			}
			rc = async_data_write_finalize(callid, next,
			    lengths[index]);
			if (rc != EOK) {
				free(*data);
				free(*strings);
				free(lengths);
				return rc;
			}
			(*strings)[index].value = next;
			next += lengths[index];
			*next++ = '\0';
		} else {
			(*strings)[index].value = NULL;
		}
	}

	free(lengths);
	return EOK;
}

/** Computes the lengths of the measured strings in the given array.
 *
 * @param[in] strings	The measured strings array to be processed.
 * @param[in] count	The measured strings array size.
 * @return		The computed sizes array.
 * @return		NULL if there is not enough memory left.
 */
static size_t *prepare_lengths(const measured_string_t *strings, size_t count)
{
	size_t *lengths;
	size_t index;
	size_t length;

	lengths = (size_t *) malloc(sizeof(size_t) * (count + 1));
	if (!lengths)
		return NULL;

	length = 0;
	for (index = 0; index < count; index++) {
		lengths[index] = strings[index].length;
		length += lengths[index] + 1;
	}
	lengths[count] = length;
	return lengths;
}

/** Replies the given measured strings array to a calling module.
 *
 * This method should be used only while processing IPC messages as the array
 * size has to be negotiated in advance.
 *
 * @param[in] strings	The measured strings array to be transferred.
 * @param[in] count	The measured strings array size.
 * @return		EOK on success.
 * @return		EINVAL if the strings parameter is NULL.
 * @return		EINVAL if the count parameter is zero (0).
 * @return		EINVAL if the calling module does not accept the given
 *			array size.
 * @return		EINVAL if there is inconsistency in sent measured
 *			strings' lengths (should not occur).
 * @return		Other error codes as defined for the
 *			async_data_read_finalize() function.
 */
int measured_strings_reply(const measured_string_t *strings, size_t count)
{
	size_t *lengths;
	size_t index;
	size_t length;
	ipc_callid_t callid;
	int rc;

	if ((!strings) || (count <= 0))
		return EINVAL;

	lengths = prepare_lengths(strings, count);
	if (!lengths)
		return ENOMEM;

	if (!async_data_read_receive(&callid, &length) ||
	    (length != sizeof(size_t) * (count + 1))) {
		free(lengths);
		return EINVAL;
	}
	rc = async_data_read_finalize(callid, lengths, length);
	if (rc != EOK) {
		free(lengths);
		return rc;
	}
	free(lengths);

	for (index = 0; index < count; index++) {
		if (strings[index].length > 0) {
			if (!async_data_read_receive(&callid, &length) ||
			    (length != strings[index].length)) {
				return EINVAL;
			}
			rc = async_data_read_finalize(callid,
			    strings[index].value, strings[index].length);
			if (rc != EOK)
				return rc;
		}
	}

	return EOK;
}

/** Receives a measured strings array from another module.
 *
 * Creates the array and the data memory blocks.
 * This method should be used only following other IPC messages as the array
 * size has to be negotiated in advance.
 *
 * @param[in] exch	Exchange.
 * @param[out] strings	The returned measured strings array.
 * @param[out] data	The measured strings data. This memory block stores the
 *			actual character strings.
 * @param[in] count	The size of the measured strings array.
 * @return		EOK on success.
 * @return		EINVAL if the strings or data parameter is NULL.
 * @return		EINVAL if the exch or count parameter is invalid.
 * @return		EINVAL if the sent array differs in size.
 * @return		ENOMEM if there is not enough memory left.
 * @return		Other error codes as defined for the
 *			async_data_read_start() function.
 */
int measured_strings_return(async_exch_t *exch, measured_string_t **strings,
    uint8_t **data, size_t count)
{
	size_t *lengths;
	size_t index;
	uint8_t *next;
	int rc;

	if ((exch == NULL) || (!strings) || (!data) || (count <= 0))
		return EINVAL;

	lengths = (size_t *) malloc(sizeof(size_t) * (count + 1));
	if (!lengths)
		return ENOMEM;

	rc = async_data_read_start(exch, lengths,
	    sizeof(size_t) * (count + 1));
	if (rc != EOK) {
		free(lengths);
		return rc;
	}

	*data = malloc(lengths[count]);
	if (!*data) {
		free(lengths);
		return ENOMEM;
	}

	*strings = (measured_string_t *) malloc(sizeof(measured_string_t) *
	    count);
	if (!*strings) {
		free(lengths);
		free(*data);
		return ENOMEM;
	}

	next = *data;
	for (index = 0; index < count; index++) {
		(*strings)[index].length = lengths[index];
		if (lengths[index] > 0) {
			rc = async_data_read_start(exch, next, lengths[index]);
			if (rc != EOK) {
			    	free(lengths);
				free(data);
				free(strings);
				return rc;
			}
			(*strings)[index].value = next;
			next += lengths[index];
			*next++ = '\0';
		} else {
			(*strings)[index].value = NULL;
		}
	}

	free(lengths);
	return EOK;
}

/** Sends the given measured strings array to another module.
 *
 * This method should be used only following other IPC messages as the array
 * size has to be negotiated in advance.
 *
 * @param[in] exch	Exchange.
 * @param[in] strings	The measured strings array to be transferred.
 * @param[in] count	The measured strings array size.
 * @return		EOK on success.
 * @return		EINVAL if the strings parameter is NULL.
 * @return		EINVAL if the exch or count parameter is invalid.
 * @return		Other error codes as defined for the
 *			async_data_write_start() function.
 */
int measured_strings_send(async_exch_t *exch, const measured_string_t *strings,
    size_t count)
{
	size_t *lengths;
	size_t index;
	int rc;

	if ((exch == NULL) || (!strings) || (count <= 0))
		return EINVAL;

	lengths = prepare_lengths(strings, count);
	if (!lengths)
		return ENOMEM;

	rc = async_data_write_start(exch, lengths,
	    sizeof(size_t) * (count + 1));
	if (rc != EOK) {
		free(lengths);
		return rc;
	}

	free(lengths);

	for (index = 0; index < count; index++) {
		if (strings[index].length > 0) {
			rc = async_data_write_start(exch, strings[index].value,
			    strings[index].length);
			if (rc != EOK)
				return rc;
		}
	}

	return EOK;
}

/** @}
 */
