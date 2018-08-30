/*
 * Copyright (c) 2012 Petr Jerman
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

/** @addtogroup libdrv
 * @{
 */
/** @file
 * @brief AHCI interface definition.
 */

#ifndef LIBDRV_AHCI_IFACE_H_
#define LIBDRV_AHCI_IFACE_H_

#include "ddf/driver.h"
#include <async.h>

extern async_sess_t *ahci_get_sess(devman_handle_t, char **);

extern errno_t ahci_get_sata_device_name(async_sess_t *, size_t, char *);
extern errno_t ahci_get_num_blocks(async_sess_t *, uint64_t *);
extern errno_t ahci_get_block_size(async_sess_t *, size_t *);
extern errno_t ahci_read_blocks(async_sess_t *, uint64_t, size_t, void *);
extern errno_t ahci_write_blocks(async_sess_t *, uint64_t, size_t, void *);

/** AHCI device communication interface. */
typedef struct {
	errno_t (*get_sata_device_name)(ddf_fun_t *, size_t, char *);
	errno_t (*get_num_blocks)(ddf_fun_t *, uint64_t *);
	errno_t (*get_block_size)(ddf_fun_t *, size_t *);
	errno_t (*read_blocks)(ddf_fun_t *, uint64_t, size_t, void *);
	errno_t (*write_blocks)(ddf_fun_t *, uint64_t, size_t, void *);
} ahci_iface_t;

#endif

/**
 * @}
 */
