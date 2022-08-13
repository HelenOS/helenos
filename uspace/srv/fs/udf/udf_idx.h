/*
 * SPDX-FileCopyrightText: 2012 Julia Medvedeva
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup udf
 * @{
 */

#ifndef UDF_IDX_H_
#define UDF_IDX_H_

#include "udf.h"

extern errno_t udf_idx_init(void);
extern errno_t udf_idx_fini(void);
extern errno_t udf_idx_get(udf_node_t **, udf_instance_t *, fs_index_t);
extern errno_t udf_idx_add(udf_node_t **, udf_instance_t *, fs_index_t);
extern errno_t udf_idx_del(udf_node_t *);

#endif /* UDF_IDX_H_ */

/**
 * @}
 */
