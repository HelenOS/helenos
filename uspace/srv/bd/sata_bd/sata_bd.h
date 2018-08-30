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

/** @addtogroup sata_bd
 * @{
 */
/** @file SATA block device driver definitions.
 */

#ifndef __SATA_BD_H__
#define __SATA_BD_H__

#define SATA_DEV_NAME_LENGTH 256

#include <async.h>
#include <bd_srv.h>
#include <loc.h>
#include <stddef.h>
#include <stdint.h>

/** SATA Block Device. */
typedef struct {
	/** Device name in device tree. */
	char *dev_name;
	/** SATA Device name. */
	char sata_dev_name[SATA_DEV_NAME_LENGTH];
	/** Session to device methods. */
	async_sess_t *sess;
	/** Loc service id. */
	service_id_t service_id;
	/** Number of blocks. */
	uint64_t blocks;
	/** Size of block. */
	size_t block_size;
	/** Block device server structure */
	bd_srvs_t bds;
} sata_bd_dev_t;

#endif

/** @}
 */
