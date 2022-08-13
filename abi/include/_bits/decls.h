/*
 * SPDX-FileCopyrightText: 2019 Jiří Zárevúcky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup bits
 * @{
 */
/** @file
 */

#ifndef _BITS_DECLS_H_
#define _BITS_DECLS_H_

#ifdef __cplusplus

#define __HELENOS_DECLS_BEGIN \
	namespace helenos { \
	extern "C" {

#define __HELENOS_DECLS_END \
	} \
	}

#define __C_DECLS_BEGIN \
	extern "C" {

#define __C_DECLS_END \
	}

#else  /* !__cplusplus */

#define __HELENOS_DECLS_BEGIN
#define __HELENOS_DECLS_END
#define __C_DECLS_BEGIN
#define __C_DECLS_END

#endif  /* __cplusplus */

#endif

/** @}
 */
