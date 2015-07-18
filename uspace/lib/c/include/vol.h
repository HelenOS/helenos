/*
 * Copyright (c) 2015 Jiri Svoboda
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

#ifndef LIBC_VOL_H_
#define LIBC_VOL_H_

#include <async.h>
#include <loc.h>
#include <stdint.h>
#include <types/label.h>

/** Volume service */
typedef struct vol {
	/** Volume service session */
	async_sess_t *sess;
} vol_t;

/** Disk information */
typedef struct {
	/** Disk contents */
	label_disk_cnt_t dcnt;
	/** Label type, if disk contents is label */
	label_type_t ltype;
	/** Label flags */
	label_flags_t flags;
} vol_disk_info_t;

extern int vol_create(vol_t **);
extern void vol_destroy(vol_t *);
extern int vol_get_disks(vol_t *, service_id_t **, size_t *);
extern int vol_disk_info(vol_t *, service_id_t, vol_disk_info_t *);
extern int vol_label_create(vol_t *, service_id_t, label_type_t);
extern int vol_disk_empty(vol_t *, service_id_t);

#endif

/** @}
 */
