/*
 * Copyright (c) 2015 Jan Kolarik
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

/** @file ath.h
 *
 * Atheros generic wifi device functions definitions.
 *
 */

#ifndef ATHEROS_ATH_H
#define ATHEROS_ATH_H

struct ath;

/** Atheros wifi operations. */
typedef struct {
	errno_t (*send_ctrl_message)(struct ath *, void *, size_t);
	errno_t (*read_ctrl_message)(struct ath *, void *, size_t, size_t *);
	errno_t (*send_data_message)(struct ath *, void *, size_t);
	errno_t (*read_data_message)(struct ath *, void *, size_t, size_t *);
} ath_ops_t;

/** Atheros wifi device structure */
typedef struct ath {
	/** Maximum length of data response message. */
	size_t data_response_length;

	/** Maximum length of control response message. */
	size_t ctrl_response_length;

	/** Implementation specific data. */
	void *specific_data;

	/** Generic Atheros wifi operations. */
	const ath_ops_t *ops;
} ath_t;

#endif  /* ATHEROS_ATH_H */
