/*
 * Copyright (c) 2007 Michal Konopa
 * Copyright (c) 2007 Martin Jelen
 * Copyright (c) 2007 Peter Majer
 * Copyright (c) 2007 Jakub Jermar
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

/** @addtogroup rd
 * @{
 */

/**
 * @file rd.c
 * @brief Initial RAM disk for HelenOS.
 */

#include <ipc/services.h>
#include <ipc/ns.h>
#include <sysinfo.h>
#include <as.h>
#include <bd_srv.h>
#include <ddi.h>
#include <align.h>
#include <stdbool.h>
#include <errno.h>
#include <str_error.h>
#include <async.h>
#include <fibril_synch.h>
#include <stdio.h>
#include <loc.h>
#include <macros.h>
#include <inttypes.h>

#define NAME  "rd"

/** Pointer to the ramdisk's image */
static void *rd_addr = AS_AREA_ANY;

/** Size of the ramdisk */
static size_t rd_size;

/** Block size */
static const size_t block_size = 512;

static errno_t rd_open(bd_srvs_t *, bd_srv_t *);
static errno_t rd_close(bd_srv_t *);
static errno_t rd_read_blocks(bd_srv_t *, aoff64_t, size_t, void *, size_t);
static errno_t rd_write_blocks(bd_srv_t *, aoff64_t, size_t, const void *, size_t);
static errno_t rd_get_block_size(bd_srv_t *, size_t *);
static errno_t rd_get_num_blocks(bd_srv_t *, aoff64_t *);

/** This rwlock protects the ramdisk's data.
 *
 * If we were to serve multiple requests (read + write or several writes)
 * concurrently (i.e. from two or more threads), each read and write needs to
 * be protected by this rwlock.
 *
 */
static fibril_rwlock_t rd_lock;

static bd_ops_t rd_bd_ops = {
	.open = rd_open,
	.close = rd_close,
	.read_blocks = rd_read_blocks,
	.write_blocks = rd_write_blocks,
	.get_block_size = rd_get_block_size,
	.get_num_blocks = rd_get_num_blocks
};

static bd_srvs_t bd_srvs;

static void rd_client_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	bd_conn(iid, icall, &bd_srvs);
}

/** Open device. */
static errno_t rd_open(bd_srvs_t *bds, bd_srv_t *bd)
{
	return EOK;
}

/** Close device. */
static errno_t rd_close(bd_srv_t *bd)
{
	return EOK;
}

/** Read blocks from the device. */
static errno_t rd_read_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt, void *buf,
    size_t size)
{
	if ((ba + cnt) * block_size > rd_size) {
		/* Reading past the end of the device. */
		return ELIMIT;
	}
	
	fibril_rwlock_read_lock(&rd_lock);
	memcpy(buf, rd_addr + ba * block_size, min(block_size * cnt, size));
	fibril_rwlock_read_unlock(&rd_lock);
	
	return EOK;
}

/** Write blocks to the device. */
static errno_t rd_write_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    const void *buf, size_t size)
{
	if ((ba + cnt) * block_size > rd_size) {
		/* Writing past the end of the device. */
		return ELIMIT;
	}
	
	fibril_rwlock_write_lock(&rd_lock);
	memcpy(rd_addr + ba * block_size, buf, min(block_size * cnt, size));
	fibril_rwlock_write_unlock(&rd_lock);
	
	return EOK;
}

/** Prepare the ramdisk image for operation. */
static bool rd_init(void)
{
	sysarg_t size;
	errno_t ret = sysinfo_get_value("rd.size", &size);
	if ((ret != EOK) || (size == 0)) {
		printf("%s: No RAM disk found\n", NAME);
		return false;
	}
	
	sysarg_t addr_phys;
	ret = sysinfo_get_value("rd.address.physical", &addr_phys);
	if ((ret != EOK) || (addr_phys == 0)) {
		printf("%s: Invalid RAM disk physical address\n", NAME);
		return false;
	}
	
	rd_size = ALIGN_UP(size, block_size);
	unsigned int flags =
	    AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE;
	
	ret = physmem_map(addr_phys,
	    ALIGN_UP(rd_size, PAGE_SIZE) >> PAGE_WIDTH, flags, &rd_addr);
	if (ret != EOK) {
		printf("%s: Error mapping RAM disk\n", NAME);
		return false;
	}
	
	printf("%s: Found RAM disk at %p, %" PRIun " bytes\n", NAME,
	    (void *) addr_phys, size);
	
	bd_srvs_init(&bd_srvs);
	bd_srvs.ops = &rd_bd_ops;
	
	async_set_fallback_port_handler(rd_client_conn, NULL);
	ret = loc_server_register(NAME);
	if (ret != EOK) {
		printf("%s: Unable to register driver: %s\n", NAME, str_error(ret));
		return false;
	}
	
	service_id_t service_id;
	ret = loc_service_register("bd/initrd", &service_id);
	if (ret != EOK) {
		printf("%s: Unable to register device service\n", NAME);
		return false;
	}
	
	fibril_rwlock_initialize(&rd_lock);
	
	return true;
}

/** Get device block size. */
static errno_t rd_get_block_size(bd_srv_t *bd, size_t *rsize)
{
	*rsize = block_size;
	return EOK;
}

/** Get number of blocks on device. */
static errno_t rd_get_num_blocks(bd_srv_t *bd, aoff64_t *rnb)
{
	*rnb = rd_size / block_size;
	return EOK;
}

int main(int argc, char **argv)
{
	printf("%s: HelenOS RAM disk server\n", NAME);
	
	if (!rd_init())
		return -1;
	
	printf("%s: Accepting connections\n", NAME);
	async_manager();
	
	/* Never reached */
	return 0;
}

/**
 * @}
 */
