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
#include <malloc.h>
#include <bool.h>

sysinfo_item_tag_t sysinfo_get_tag(const char *path)
{
	return (sysinfo_item_tag_t) __SYSCALL2(SYS_SYSINFO_GET_TAG,
	    (sysarg_t) path, (sysarg_t) str_size(path));
}

int sysinfo_get_value(const char *path, sysarg_t *value)
{
	return (int) __SYSCALL3(SYS_SYSINFO_GET_VALUE, (sysarg_t) path,
	    (sysarg_t) str_size(path), (sysarg_t) value);
}

static int sysinfo_get_data_size(const char *path, size_t *size)
{
	return (int) __SYSCALL3(SYS_SYSINFO_GET_DATA_SIZE, (sysarg_t) path,
	    (sysarg_t) str_size(path), (sysarg_t) size);
}

void *sysinfo_get_data(const char *path, size_t *size)
{
	while (true) {
		int ret = sysinfo_get_data_size(path, size);
		if (ret != EOK)
			return NULL;
		
		void *data = malloc(*size);
		if (data == NULL)
			return NULL;
		
		ret = __SYSCALL4(SYS_SYSINFO_GET_DATA, (sysarg_t) path,
		    (sysarg_t) str_size(path), (sysarg_t) data, (sysarg_t) *size);
		if (ret == EOK)
			return data;
		
		free(data);
		
		if (ret != ENOMEM)
			return NULL;
	}
}

/** @}
 */
