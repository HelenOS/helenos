/*
 * SPDX-FileCopyrightText: 2015 Jan Kolarik
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
