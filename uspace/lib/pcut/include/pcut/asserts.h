/*
 * Copyright (c) 2014 Vojtech Horky
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

/** @file
 * Predefined asserts.
 *
 * @defgroup asserts Asserts
 * Predefined asserts you can use with PCUT.
 * @{
 */
#ifndef PCUT_ASSERTS_H_GUARD
#define PCUT_ASSERTS_H_GUARD

#include <errno.h>

/** @def PCUT_CURRENT_FILENAME
 * Overwrite contents of __FILE__ when printing assertion errors.
 */
#ifndef PCUT_CURRENT_FILENAME
#define PCUT_CURRENT_FILENAME __FILE__
#endif

/** @cond devel */

/** Raise assertion error.
 *
 * This function immediately terminates current test and executes a tear-down
 * (if registered).
 *
 * @param filename File where the assertion occurred.
 * @param line Line where the assertion occurred.
 * @param fmt Printf-like format string.
 * @param ... Extra arguments.
 */
void pcut_failed_assertion_fmt(const char *filename, int line, const char *fmt, ...);

/** OS-agnostic string comparison.
 *
 * @param a First string to compare.
 * @param b Second string to compare.
 * @return Whether the strings are equal.
 */
int pcut_str_equals(const char *a, const char *b);

/** OS-agnostic conversion from error code to error description.
 *
 * This function ensures that the description stored in @p buffer is
 * always a nul-terminated string.
 *
 * @param error Error code.
 * @param buffer Where to store the error description.
 * @param size Size of the buffer.
 */
void pcut_str_error(int error, char *buffer, int size);

/** Raise assertion error (internal version).
 *
 * We expect to be always called from PCUT_ASSERTION_FAILED() where
 * the last argument is empty string to conform to strict ISO C99
 * ("ISO C99 requires rest arguments to be used").
 *
 * @param fmt Printf-like format string.
 * @param ... Extra arguments.
 */
#define PCUT_ASSERTION_FAILED_INTERNAL(fmt, ...) \
	pcut_failed_assertion_fmt(PCUT_CURRENT_FILENAME, __LINE__, fmt, __VA_ARGS__)


/** @endcond */

/** Raise assertion error.
 *
 * This macro adds current file and line and triggers immediate test
 * abort (tear-down function of the test suite is run when available).
 *
 * @param ... Printf-like arguments.
 */
#define PCUT_ASSERTION_FAILED(...) \
	PCUT_ASSERTION_FAILED_INTERNAL(__VA_ARGS__, "")


/** Generic assertion for a boolean expression.
 *
 * @param actual Actually obtained (computed) value we wish to test to true.
 */
#define PCUT_ASSERT_TRUE(actual) \
	do { \
		if (!(actual)) { \
			PCUT_ASSERTION_FAILED("Expected true but got <%s>", \
				#actual); \
		} \
	} while (0)

/** Generic assertion for a boolean expression.
 *
 * @param actual Actually obtained (computed) value we wish to test to false.
 */
#define PCUT_ASSERT_FALSE(actual) \
	do { \
		if ((actual)) { \
			PCUT_ASSERTION_FAILED("Expected false but got <%s>", \
				#actual); \
		} \
	} while (0)


/** Generic assertion for types where == is defined.
 *
 * @param expected Expected (correct) value.
 * @param actual Actually obtained (computed) value we wish to test.
 */
#define PCUT_ASSERT_EQUALS(expected, actual) \
	do {\
		if (!((expected) == (actual))) { \
			PCUT_ASSERTION_FAILED("Expected <"#expected "> but got <" #actual ">"); \
		} \
	} while (0)

/** Asserts that given pointer is NULL.
 *
 * @param pointer The pointer to be tested.
 */
#define PCUT_ASSERT_NULL(pointer) \
	do { \
		const void *pcut_ptr_eval = (pointer); \
		if (pcut_ptr_eval != (NULL)) { \
			PCUT_ASSERTION_FAILED("Expected <" #pointer "> to be NULL, " \
				"instead it points to %p", pcut_ptr_eval); \
		} \
	} while (0)

/** Asserts that given pointer is not NULL.
 *
 * Use this function when directly quoting the original pointer variable
 * does not provide sufficient information.
 *
 * @param pointer The pointer to be tested.
 * @param pointer_name Name of the pointer.
 */
#define PCUT_ASSERT_NOT_NULL_WITH_NAME(pointer, pointer_name) \
	do { \
		const void *pcut_ptr_eval = (pointer); \
		if (pcut_ptr_eval == (NULL)) { \
			PCUT_ASSERTION_FAILED("Pointer <" pointer_name "> ought not to be NULL"); \
		} \
	} while (0)

/** Asserts that given pointer is not NULL.
 *
 * @param pointer The pointer to be tested.
 */
#define PCUT_ASSERT_NOT_NULL(pointer) \
	PCUT_ASSERT_NOT_NULL_WITH_NAME(pointer, #pointer)


/** Assertion for checking that two integers are equal.
 *
 * @param expected Expected (correct) value.
 * @param actual Actually obtained (computed) value we wish to test.
 */
#define PCUT_ASSERT_INT_EQUALS(expected, actual) \
	do {\
		long long pcut_expected_eval = (expected); \
		long long pcut_actual_eval = (actual); \
		if (pcut_expected_eval != pcut_actual_eval) { \
			PCUT_ASSERTION_FAILED("Expected <%lld> but got <%lld> (%s != %s)", \
				pcut_expected_eval, pcut_actual_eval, \
				#expected, #actual); \
		} \
	} while (0)

/** Assertion for checking that two integers are equal.
 *
 * @param expected Expected (correct) value.
 * @param actual Actually obtained (computed) value we wish to test.
 */
#define PCUT_ASSERT_UINT_EQUALS(expected, actual) \
	do {\
		unsigned long long pcut_expected_eval = (expected); \
		unsigned long long pcut_actual_eval = (actual); \
		if (pcut_expected_eval != pcut_actual_eval) { \
			PCUT_ASSERTION_FAILED("Expected <%llu> but got <%llu> (%s != %s)", \
				pcut_expected_eval, pcut_actual_eval, \
				#expected, #actual); \
		} \
	} while (0)

/** Assertion for checking that two pointers are equal.
 *
 * @param expected Expected (correct) value.
 * @param actual Actually obtained (computed) value we wish to test.
 */
#define PCUT_ASSERT_PTR_EQUALS(expected, actual) \
	do {\
		const void *pcut_expected_eval = (expected); \
		const void *pcut_actual_eval = (actual); \
		if (pcut_expected_eval != pcut_actual_eval) { \
			PCUT_ASSERTION_FAILED("Expected '" #actual "' = '" #expected "' = <%p> but got '" #actual "' = <%p>", \
				pcut_expected_eval, pcut_actual_eval); \
		} \
	} while (0)

/** Assertion for checking that two doubles are close enough.
 *
 * @param expected Expected (correct) value.
 * @param actual Actually obtained (computed) value we wish to test.
 * @param epsilon How much the actual value can differ from the expected one.
 */
#define PCUT_ASSERT_DOUBLE_EQUALS(expected, actual, epsilon) \
	do {\
		double pcut_expected_eval = (expected); \
		double pcut_actual_eval = (actual); \
		double pcut_epsilon_eval = (epsilon); \
		double pcut_double_diff = pcut_expected_eval - pcut_actual_eval; \
		if ((pcut_double_diff < -pcut_epsilon_eval) | (pcut_double_diff > pcut_epsilon_eval)) { \
			PCUT_ASSERTION_FAILED("Expected <%lf+-%lf> but got <%lf> (%s != %s)", \
				pcut_expected_eval, pcut_epsilon_eval, pcut_actual_eval, \
				#expected, #actual); \
		} \
	} while (0)

/** Assertion for checking that two strings (`const char *`) are equal.
 *
 * @param expected Expected (correct) value.
 * @param actual Actually obtained (computed) value we wish to test.
 */
#define PCUT_ASSERT_STR_EQUALS(expected, actual) \
	do {\
		const char *pcut_expected_eval = (expected); \
		const char *pcut_actual_eval = (actual); \
		PCUT_ASSERT_NOT_NULL_WITH_NAME(pcut_expected_eval, #expected); \
		PCUT_ASSERT_NOT_NULL_WITH_NAME(pcut_actual_eval, #actual); \
		if (!pcut_str_equals(pcut_expected_eval, pcut_actual_eval)) { \
			PCUT_ASSERTION_FAILED("Expected <%s> but got <%s> (%s != %s)", \
				pcut_expected_eval, pcut_actual_eval, \
				#expected, #actual); \
		} \
	} while (0)


/** Assertion for checking that two strings (`const char *`) are equal or both NULL.
 *
 * @param expected Expected (correct) value.
 * @param actual Actually obtained (computed) value we wish to test.
 */
#define PCUT_ASSERT_STR_EQUALS_OR_NULL(expected, actual) \
	do {\
		const char *pcut_expected_eval = (expected); \
		const char *pcut_actual_eval = (actual); \
		int pcut_both_null = (pcut_expected_eval == NULL) && (pcut_actual_eval == NULL); \
		int pcut_some_null = (pcut_expected_eval == NULL) || (pcut_actual_eval == NULL); \
		if (!pcut_both_null && (pcut_some_null || !pcut_str_equals(pcut_expected_eval, pcut_actual_eval))) { \
			PCUT_ASSERTION_FAILED("Expected <%s> but got <%s> (%s != %s)", \
				pcut_expected_eval == NULL ? "NULL" : pcut_expected_eval, \
				pcut_actual_eval == NULL ? "NULL" : pcut_actual_eval, \
				#expected, #actual); \
		} \
	} while (0)


/** Assertion for checking errno-style variables for errors.
 *
 * Use this function when directly quoting the original error code does
 * not provide sufficient information.
 *
 * @param expected_value Expected errno error code.
 * @param expected_quoted Expected error code as a string.
 * @param actual_value Actual value of the error code.
 */
#define PCUT_ASSERT_ERRNO_VAL_WITH_NAME(expected_value, expected_quoted, actual_value) \
	do {\
		int pcut_expected_eval = (expected_value); \
		int pcut_actual_eval = (actual_value); \
		if (pcut_expected_eval != pcut_actual_eval) { \
			char pcut_expected_description[100]; \
			char pcut_actual_description[100]; \
			pcut_str_error(pcut_expected_eval, pcut_expected_description, 100); \
			pcut_str_error(pcut_actual_eval, pcut_actual_description, 100); \
			PCUT_ASSERTION_FAILED("Expected error %d (%s, %s) but got error %d (%s)", \
				pcut_expected_eval, expected_quoted, \
				pcut_expected_description, \
				pcut_actual_eval, pcut_actual_description); \
		} \
	} while (0)

/** Assertion for checking errno-style variables for errors.
 *
 * @param expected Expected errno error code.
 * @param actual Actual value of the error code.
 */
#define PCUT_ASSERT_ERRNO_VAL(expected, actual) \
	PCUT_ASSERT_ERRNO_VAL_WITH_NAME(expected, #expected, actual)

/** Assertion for checking errno variable for errors.
 *
 * @param expected Expected errno error code.
 */
#define PCUT_ASSERT_ERRNO(expected) \
	PCUT_ASSERT_ERRNO_VAL_WITH_NAME(expected, #expected, (errno))

/**
 * @}
 */

#endif
