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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <async.h>
#include <ipc/services.h>
#include <ipc/ns.h>
#include <sysinfo.h>
#include <errno.h>
#include <as.h>
#include <macros.h>

int service_register(sysarg_t service)
{
	return async_connect_to_me(PHONE_NS, service, 0, 0, NULL);
}

int service_connect(sysarg_t service, sysarg_t arg2, sysarg_t arg3)
{
	return async_connect_me_to(PHONE_NS, service, arg2, arg3);
}

int service_connect_blocking(sysarg_t service, sysarg_t arg2, sysarg_t arg3)
{
	return async_connect_me_to_blocking(PHONE_NS, service, arg2, arg3);
}

wchar_t *service_klog_share_in(size_t *length)
{
	size_t pages;
	if (sysinfo_get_value("klog.pages", &pages) != EOK)
		return NULL;
	
	size_t size = pages * PAGE_SIZE;
	*length = size / sizeof(wchar_t);
	
	wchar_t *klog = (wchar_t *) as_get_mappable_page(size);
	if (klog == NULL)
		return NULL;
	
	int res = async_share_in_start_1_0(PHONE_NS, (void *) klog, size,
	    SERVICE_MEM_KLOG);
	if (res != EOK) {
		as_area_destroy((void *) klog);
		return NULL;
	}
	
	return klog;
}

void *service_realtime_share_in(void)
{
	void *rtime = as_get_mappable_page(PAGE_SIZE);
	if (rtime == NULL)
		return NULL;
	
	int res = async_share_in_start_1_0(PHONE_NS, rtime, PAGE_SIZE,
	    SERVICE_MEM_REALTIME);
	if (res != EOK) {
		as_area_destroy((void *) rtime);
		return NULL;
	}
	
	return rtime;
}

/** @}
 */
