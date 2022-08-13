/*
 * SPDX-FileCopyrightText: 2018 CZ.NIC, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FENV_H
#define _FENV_H

// TODO

#define FE_TOWARDZERO  0
#define FE_TONEAREST   1
#define FE_UPWARD      2
#define FE_DOWNWARD    3

#define fegetround() FE_TONEAREST

#endif
