/*
 * Copyright (c) 2025 Jiri Svoboda
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

/** @addtogroup fmgt
 * @{
 */
/**
 * @file
 * @brief File managment library.
 */

#ifndef TYPES_FMGT_H
#define TYPES_FMGT_H

#include <adt/list.h>
#include <capa.h>
#include <errno.h>
#include <fibril_synch.h>
#include <stdbool.h>

#define FMGT_FILE_COUNT_BUFSIZE 16

/** File management progress update */
typedef struct {
	/** Current file processed bytes */
	char curf_procb[CAPA_BLOCKS_BUFSIZE];
	/** Total bytes to process for current file */
	char curf_totalb[CAPA_BLOCKS_BUFSIZE];
	/** Percent of current file processed */
	char curf_percent[5];
	/** Total number of bytes processed */
	char total_procb[CAPA_BLOCKS_BUFSIZE];
	/** Number of files processed */
	char total_procf[FMGT_FILE_COUNT_BUFSIZE];
} fmgt_progress_t;

/** File management I/O operation type */
typedef enum {
	/** Read */
	fmgt_io_read,
	/** Write */
	fmgt_io_write,
	/** Open */
	fmgt_io_open,
	/** Create */
	fmgt_io_create
} fmgt_io_op_type_t;

/** File management I/O error report */
typedef struct {
	/** File name */
	const char *fname;
	/** Operation type */
	fmgt_io_op_type_t optype;
	/** Error code */
	errno_t rc;
} fmgt_io_error_t;

/** File management I/O error recovery action */
typedef enum {
	/** Retry */
	fmgt_er_retry,
	/** Abort */
	fmgt_er_abort
} fmgt_error_action_t;

/** File management callbacks */
typedef struct {
	bool (*abort_query)(void *);
	fmgt_error_action_t (*io_error_query)(void *, fmgt_io_error_t *);
	void (*progress)(void *, fmgt_progress_t *);
} fmgt_cb_t;

typedef struct {
	/** Lock */
	fibril_mutex_t lock;
	/** Progress update timer */
	fibril_timer_t *timer;
	/** Callback functions */
	fmgt_cb_t *cb;
	/** Argument to callback functions */
	void *cb_arg;
	/** Bytes processed from current file */
	uint64_t curf_procb;
	/** Total size of current file */
	uint64_t curf_totalb;
	/** Progress was displayed for current file */
	bool curf_progr;
	/** Total number of processed bytes */
	uint64_t total_procb;
	/** Total number of processed files */
	unsigned total_procf;
	/** Post an immediate initial progress update */
	bool do_init_update;
} fmgt_t;

/** New file flags */
typedef enum {
	nf_none = 0x0,
	nf_sparse = 0x1
} fmgt_nf_flags_t;

/** File list */
typedef struct {
	/** List of fmgt_flist_entry_t */
	list_t files;
} fmgt_flist_t;

/** File list entry */
typedef struct {
	/** Containing file list */
	fmgt_flist_t *flist;
	/** Link to flist->files */
	link_t lfiles;
	/** File name */
	char *fname;
} fmgt_flist_entry_t;

/** File system tree walk callbacks */
typedef struct {
	errno_t (*dir_enter)(void *, const char *, const char *);
	errno_t (*dir_leave)(void *, const char *, const char *);
	errno_t (*file)(void *, const char *, const char *);
} fmgt_walk_cb_t;

/** File system tree walk parameters */
typedef struct {
	/** List of files or directories (walk roots) */
	fmgt_flist_t *flist;
	/** Destination path */
	const char *dest;
	/** Copy files into destination directory. */
	bool into_dest;
	/** Callbacks */
	fmgt_walk_cb_t *cb;
	/** Callback argument */
	void *arg;
} fmgt_walk_params_t;

#endif

/** @}
 */
