/*
 * SPDX-FileCopyrightText: 2017 Petr Manek
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup tmon
 * @{
 */
/**
 * @file USB diagnostic device commands.
 */

#ifndef TMON_COMMANDS_H_
#define TMON_COMMANDS_H_

/* All commands are just versions of int main(int, char **). */

/* List command just prints compatible devices. */
int tmon_list(int, char **);

/* Tests commands differ by endpoint types. */
int tmon_test_intr_in(int, char **);
int tmon_test_intr_out(int, char **);
int tmon_test_bulk_in(int, char **);
int tmon_test_bulk_out(int, char **);
int tmon_test_isoch_in(int, char **);
int tmon_test_isoch_out(int, char **);

#endif /* TMON_COMMANDS_H_ */

/** @}
 */
