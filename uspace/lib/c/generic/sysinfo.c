/*
 * Copyright (c) 2006 Jakub Jermar
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
 * @{
 */
/** @file
 */

#include <libc.h>
#include <sysinfo.h>
#include <str.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>

/** Get sysinfo keys size
 *
 * @param path  Sysinfo path.
 * @param value Pointer to store the keys size.
 *
 * @return EOK if the keys were successfully read.
 *
 */
static errno_t sysinfo_get_keys_size(const char *path, size_t *size)
{
	return (errno_t) __SYSCALL3(SYS_SYSINFO_GET_KEYS_SIZE, (sysarg_t) path,
	    (sysarg_t) str_size(path), (sysarg_t) size);
}

/** Get sysinfo keys
 *
 * @param path  Sysinfo path.
 * @param value Pointer to store the keys size.
 *
 * @return Keys read from sysinfo or NULL if the
 *         sysinfo item has no subkeys.
 *         The returned non-NULL pointer should be
 *         freed by free().
 *
 */
char *sysinfo_get_keys(const char *path, size_t *size)
{
	/*
	 * The size of the keys might change during time.
	 * Unfortunatelly we cannot allocate the buffer
	 * and transfer the keys as a single atomic operation.
	 */

	/* Get the keys size */
	errno_t ret = sysinfo_get_keys_size(path, size);
	if ((ret != EOK) || (size == 0)) {
		/*
		 * Item with no subkeys.
		 */
		*size = 0;
		return NULL;
	}

	char *data = malloc(*size);
	if (data == NULL) {
		*size = 0;
		return NULL;
	}

	/* Get the data */
	size_t sz;
	ret = (errno_t) __SYSCALL5(SYS_SYSINFO_GET_KEYS, (sysarg_t) path,
	    (sysarg_t) str_size(path), (sysarg_t) data, (sysarg_t) *size,
	    (sysarg_t) &sz);
	if (ret == EOK) {
		*size = sz;
		return data;
	}

	free(data);
	*size = 0;
	return NULL;
}

/** Get sysinfo item type
 *
 * @param path Sysinfo path.
 *
 * @return Sysinfo item type.
 *
 */
sysinfo_item_val_type_t sysinfo_get_val_type(const char *path)
{
	return (sysinfo_item_val_type_t) __SYSCALL2(SYS_SYSINFO_GET_VAL_TYPE,
	    (sysarg_t) path, (sysarg_t) str_size(path));
}

/** Get sysinfo numerical value
 *
 * @param path  Sysinfo path.
 * @param value Pointer to store the numerical value to.
 *
 * @return EOK if the value was successfully read and
 *         is of SYSINFO_VAL_VAL type.
 *
 */
errno_t sysinfo_get_value(const char *path, sysarg_t *value)
{
	return (errno_t) __SYSCALL3(SYS_SYSINFO_GET_VALUE, (sysarg_t) path,
	    (sysarg_t) str_size(path), (sysarg_t) value);
}

/** Get sysinfo binary data size
 *
 * @param path Sysinfo path.
 * @param size Pointer to store the binary data size.
 *
 * @return EOK if the value was successfully read and
 *         is of SYSINFO_VAL_DATA type.
 *
 */
static errno_t sysinfo_get_data_size(const char *path, size_t *size)
{
	return (errno_t) __SYSCALL3(SYS_SYSINFO_GET_DATA_SIZE, (sysarg_t) path,
	    (sysarg_t) str_size(path), (sysarg_t) size);
}

/** Get sysinfo binary data
 *
 * @param path Sysinfo path.
 * @param size Pointer to store the binary data size.
 *
 * @return Binary data read from sysinfo or NULL if the
 *         sysinfo item value type is not binary data.
 *         The returned non-NULL pointer should be
 *         freed by free().
 *
 */
void *sysinfo_get_data(const char *path, size_t *size)
{
	/*
	 * The binary data size might change during time.
	 * Unfortunatelly we cannot allocate the buffer
	 * and transfer the data as a single atomic operation.
	 */

	/* Get the binary data size */
	errno_t ret = sysinfo_get_data_size(path, size);
	if ((ret != EOK) || (size == 0)) {
		/*
		 * Not a binary data item
		 * or an empty item.
		 */
		*size = 0;
		return NULL;
	}

	void *data = malloc(*size);
	if (data == NULL) {
		*size = 0;
		return NULL;
	}

	/* Get the data */
	size_t sz;
	ret = (errno_t) __SYSCALL5(SYS_SYSINFO_GET_DATA, (sysarg_t) path,
	    (sysarg_t) str_size(path), (sysarg_t) data, (sysarg_t) *size,
	    (sysarg_t) &sz);
	if (ret == EOK) {
		*size = sz;
		return data;
	}

	free(data);
	*size = 0;
	return NULL;
}

/** Get sysinfo property
 *
 * @param path Sysinfo path.
 * @param name Property name.
 * @param size Pointer to store the binary data size.
 *
 * @return Property value read from sysinfo or NULL if the
 *         sysinfo item value type is not binary data.
 *         The returned non-NULL pointer should be
 *         freed by free().
 *
 */
void *sysinfo_get_property(const char *path, const char *name, size_t *size)
{
	size_t total_size;
	void *data = sysinfo_get_data(path, &total_size);
	if ((data == NULL) || (total_size == 0)) {
		*size = 0;
		return NULL;
	}

	size_t pos = 0;
	while (pos < total_size) {
		/* Process each property with sanity checks */
		size_t cur_size = str_nsize(data + pos, total_size - pos);
		if (((char *) data)[pos + cur_size] != 0)
			break;

		bool found = (str_cmp(data + pos, name) == 0);

		pos += cur_size + 1;
		if (pos >= total_size)
			break;

		/* Process value size */
		size_t value_size;
		memcpy(&value_size, data + pos, sizeof(value_size));

		pos += sizeof(value_size);
		if ((pos >= total_size) || (pos + value_size > total_size))
			break;

		if (found) {
			void *value = malloc(value_size);
			if (value == NULL)
				break;

			memcpy(value, data + pos, value_size);
			free(data);

			*size = value_size;
			return value;
		}

		pos += value_size;
	}

	free(data);

	*size = 0;
	return NULL;
}

/** @}
 */
