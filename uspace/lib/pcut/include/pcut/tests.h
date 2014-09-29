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
 * Tests and test suites.
 *
 * @defgroup tests Tests
 * Create test suites and test cases.
 * @{
 */
#ifndef PCUT_TESTS_H_GUARD
#define PCUT_TESTS_H_GUARD

#include <pcut/helper.h>
#include <pcut/datadef.h>

/*
 * Trick with [] and & copied from
 * http://bytes.com/topic/c/answers/553555-initializer-element-not-constant#post2159846
 * because following does not work:
 * extern int *a;
 * int *b = a;
 */


#ifndef __COUNTER__
#define PCUT_WITHOUT_COUNTER
#endif

#ifndef PCUT_WITHOUT_COUNTER
#define PCUT_ITEM_COUNTER_INCREMENT
#endif


/** Default timeout for a single test (in seconds).
 * @showinitializer
 */
#define PCUT_DEFAULT_TEST_TIMEOUT 3

/** Item counter that is expected to be incremented by preprocessor.
 *
 * Used compiler must support __COUNTER__ for this to work without any
 * extra preprocessing.
 */
#ifdef PCUT_WITHOUT_COUNTER
#define PCUT_ITEM_COUNTER PCUT_you_need_to_run_pcut_preprocessor
#else
#define PCUT_ITEM_COUNTER __COUNTER__
#endif


/*
 * Helper macros
 * -------------
 */

/** @cond devel */

/** Produce identifier name for an item with given number.
 *
 * @param number Item number.
 */
#ifndef PCUT_WITHOUT_COUNTER
#define PCUT_ITEM_NAME(number) \
	PCUT_JOIN(pcut_item_, number)
#else
#define PCUT_ITEM_NAME(number) PCUT_ITEM_NAME
#endif

/** Produce identifier name for a preceding item.
 *
 * @param number Number of the current item.
 */
#ifndef PCUT_WITHOUT_COUNTER
#define PCUT_ITEM_NAME_PREV(number) \
	PCUT_JOIN(pcut_item_, PCUT_JOIN(PCUT_PREV_, number))
#else
#define PCUT_ITEM_NAME_PREV(number) PCUT_ITEM_NAME_PREV
#endif

#ifndef PCUT_WITHOUT_COUNTER
#define PCUT_ITEM_EXTRAS_NAME(number) \
	PCUT_JOIN(pcut_extras_, number)
#else
#define PCUT_ITEM_EXTRAS_NAME(number) PCUT_ITEM2_NAME
#endif

#ifndef PCUT_WITHOUT_COUNTER
#define PCUT_ITEM_SETUP_NAME(number) \
	PCUT_JOIN(pcut_setup_, number)
#else
#define PCUT_ITEM_SETUP_NAME(number) PCUT_ITEM3_NAME
#endif



/** Create a new item, append it to the list.
 *
 * @param number Number of this item.
 * @param itemkind Kind of this item (PCUT_KIND_*).
 * @param ... Other initializers of the pcut_item_t.
 */
#define PCUT_ADD_ITEM(number, itemkind, ...) \
	static pcut_item_t PCUT_ITEM_NAME(number) = { \
		&PCUT_ITEM_NAME_PREV(number), \
		NULL, \
		-1, \
		itemkind, \
		__VA_ARGS__ \
	}

/** @endcond */



/*
 * Test-case related macros
 * ------------------------
 */

/** Define test time-out.
 *
 * Use as argument to PCUT_TEST().
 *
 * @param time_out Time-out value in seconds.
 */
#define PCUT_TEST_SET_TIMEOUT(time_out) \
	{ PCUT_EXTRA_TIMEOUT, (time_out) }

/** Skip current test.
 *
 * Use as argument to PCUT_TEST().
 */
#define PCUT_TEST_SKIP \
	{ PCUT_EXTRA_SKIP, 0 }


/** @cond devel */

/** Terminate list of extra test options. */
#define PCUT_TEST_EXTRA_LAST { PCUT_EXTRA_LAST, 0 }

/** Define a new test with given name and given item number.
 *
 * @param testname A valid C identifier name (not quoted).
 * @param number Number of the item describing this test.
 * @param ... Extra test properties.
 */
#define PCUT_TEST_WITH_NUMBER(number, testname, ...) \
	PCUT_ITEM_COUNTER_INCREMENT \
	static pcut_extra_t PCUT_ITEM_EXTRAS_NAME(number)[] = { \
		__VA_ARGS__ \
	}; \
	static int PCUT_CC_UNUSED_VARIABLE(PCUT_JOIN(testname, 0_test_name_missing_or_duplicated), 0); \
	static void PCUT_JOIN(test_, testname)(void); \
	PCUT_ADD_ITEM(number, PCUT_KIND_TEST, \
		PCUT_QUOTE(testname), \
		PCUT_JOIN(test_, testname), \
		NULL, NULL, \
		PCUT_ITEM_EXTRAS_NAME(number), \
		NULL, NULL \
	); \
	void PCUT_JOIN(test_, testname)(void)

/** @endcond */

/** Define a new test with given name.
 *
 * @param ... Test name (C identifier) followed by extra test properties.
 */
#define PCUT_TEST(...) \
	PCUT_TEST_WITH_NUMBER(PCUT_ITEM_COUNTER, \
		PCUT_VARG_GET_FIRST(__VA_ARGS__, this_arg_is_ignored), \
		PCUT_VARG_SKIP_FIRST(__VA_ARGS__, PCUT_TEST_EXTRA_LAST) \
	)





/*
 * Test suite related macros
 * -------------------------
 */

/** @cond devel */

/** Define and start a new test suite.
 *
 * @see PCUT_TEST_SUITE
 *
 * @param suitename Suite name (a valid C identifier).
 * @param number Item number.
 */
#define PCUT_TEST_SUITE_WITH_NUMBER(suitename, number) \
	PCUT_ITEM_COUNTER_INCREMENT \
	PCUT_ADD_ITEM(number, PCUT_KIND_TESTSUITE, \
		#suitename, \
		NULL, \
		NULL, NULL, \
		NULL, NULL, \
		NULL \
	)

/** Define a set-up function for a test suite.
 *
 * @see PCUT_TEST_BEFORE
 *
 * @param number Item number.
 */
#define PCUT_TEST_BEFORE_WITH_NUMBER(number) \
	PCUT_ITEM_COUNTER_INCREMENT \
	static void PCUT_ITEM_SETUP_NAME(number)(void); \
	PCUT_ADD_ITEM(number, PCUT_KIND_SETUP, \
		"setup", NULL, \
		PCUT_ITEM_SETUP_NAME(number), \
		NULL, NULL, NULL, NULL \
	); \
	void PCUT_ITEM_SETUP_NAME(number)(void)

/** Define a tear-down function for a test suite.
 *
 * @see PCUT_TEST_AFTER
 *
 * @param number Item number.
 */
#define PCUT_TEST_AFTER_WITH_NUMBER(number) \
	PCUT_ITEM_COUNTER_INCREMENT \
	static void PCUT_ITEM_SETUP_NAME(number)(void); \
	PCUT_ADD_ITEM(number, PCUT_KIND_TEARDOWN, \
		"teardown", NULL, NULL, \
		PCUT_ITEM_SETUP_NAME(number), \
		NULL, NULL, NULL \
	); \
	void PCUT_ITEM_SETUP_NAME(number)(void)

/** @endcond */


/** Define and start a new test suite.
 *
 * All tests following this macro belong to the new suite
 * (up to next occurrence of PCUT_TEST_SUITE).
 *
 * This command shall be used as is without any extra code.
 *
 * @param name Suite name (a valid C identifier).
 */
#define PCUT_TEST_SUITE(name) \
	PCUT_TEST_SUITE_WITH_NUMBER(name, PCUT_ITEM_COUNTER)

/** Define a set-up function for a test suite.
 *
 * The code of the function immediately follows this macro.
 *
 * @code
 * PCUT_TEST_SUITE(my_suite);
 *
 * PCUT_TEST_BEFORE {
 *     printf("This is executed before each test in this suite.\n");
 * }
 * @endcode
 *
 * There could be only a single set-up function for each suite.
 */
#define PCUT_TEST_BEFORE \
	PCUT_TEST_BEFORE_WITH_NUMBER(PCUT_ITEM_COUNTER)

/** Define a tear-down function for a test suite.
 *
 * The code of the function immediately follows this macro.
 *
 * @code
 * PCUT_TEST_SUITE(my_suite);
 *
 * PCUT_TEST_AFTER {
 *     printf("This is executed after each test in this suite.\n");
 * }
 * @endcode
 *
 * There could be only a single tear-down function for each suite.
 */
#define PCUT_TEST_AFTER \
	PCUT_TEST_AFTER_WITH_NUMBER(PCUT_ITEM_COUNTER)





/*
 * Import/export related macros
 * ----------------------------
 */

/** @cond devel */

/** Export test cases from current file.
 *
 * @see PCUT_EXPORT
 *
 * @param identifier Identifier of this block of tests.
 * @param number Item number.
 */
#define PCUT_EXPORT_WITH_NUMBER(identifier, number) \
	PCUT_ITEM_COUNTER_INCREMENT \
	pcut_item_t pcut_exported_##identifier = { \
		&PCUT_ITEM_NAME_PREV(number), \
		NULL, \
		-1, \
		PCUT_KIND_SKIP, \
		"exported_" #identifier, NULL, NULL, NULL, NULL, NULL, NULL \
	}

/** Import test cases from a different file.
 *
 * @see PCUT_EXPORT
 *
 * @param identifier Identifier of the tests to import.
 * @param number Item number.
 */
#define PCUT_IMPORT_WITH_NUMBER(identifier, number) \
	PCUT_ITEM_COUNTER_INCREMENT \
	extern pcut_item_t pcut_exported_##identifier; \
	PCUT_ADD_ITEM(number, PCUT_KIND_NESTED, \
		"import_" #identifier, NULL, NULL, NULL, NULL, NULL, \
		&pcut_exported_##identifier \
	)

/** @endcond */

/** Export test cases from current file.
 *
 * @param identifier Identifier of this block of tests.
 */
#define PCUT_EXPORT(identifier) \
	PCUT_EXPORT_WITH_NUMBER(identifier, PCUT_ITEM_COUNTER)

/** Import test cases from a different file.
 *
 * @param identifier Identifier of the tests to import.
 * (previously exported with PCUT_EXPORT).
 */
#define PCUT_IMPORT(identifier) \
	PCUT_IMPORT_WITH_NUMBER(identifier, PCUT_ITEM_COUNTER)





/*
 * PCUT initialization and invocation macros
 * -----------------------------------------
 */

/** @cond devel */

/** Initialize the PCUT testing framework with a first item.
 *
 * @param first_number Number of the first item.
 */
#define PCUT_INIT_WITH_NUMBER(first_number) \
	PCUT_ITEM_COUNTER_INCREMENT \
	static pcut_item_t PCUT_ITEM_NAME(first_number) = { \
		NULL, \
		NULL, \
		-1, \
		PCUT_KIND_SKIP, \
		"init", NULL, NULL, NULL, NULL, NULL, NULL \
	}; \
	PCUT_TEST_SUITE(Default);

int pcut_main(pcut_item_t *last, int argc, char *argv[]);

/** Insert code to run all the tests.
 *
 * @param number Item number.
 */
#define PCUT_MAIN_WITH_NUMBER(number, ...) \
	PCUT_ITEM_COUNTER_INCREMENT \
	static pcut_main_extra_t pcut_main_extras[] = { \
		__VA_ARGS__ \
	}; \
	static pcut_item_t pcut_item_last = { \
		&PCUT_ITEM_NAME_PREV(number), \
		NULL, \
		-1, \
		PCUT_KIND_SKIP, \
		"main", NULL, NULL, NULL, \
		NULL, \
		pcut_main_extras, \
		NULL \
	}; \
	int main(int argc, char *argv[]) { \
		return pcut_main(&pcut_item_last, argc, argv); \
	}

/** Terminate list of extra options for main. */
#define PCUT_MAIN_EXTRA_SET_LAST \
	{ PCUT_MAIN_EXTRA_LAST, NULL, NULL }

/** @endcond */

/** Initialize the PCUT testing framework. */
#define PCUT_INIT \
	PCUT_INIT_WITH_NUMBER(PCUT_ITEM_COUNTER)

/** Insert code to run all the tests. */
#define PCUT_MAIN() \
	PCUT_MAIN_WITH_NUMBER(PCUT_ITEM_COUNTER, PCUT_MAIN_EXTRA_SET_LAST)


/** Set callback for PCUT initialization.
 *
 * Use from within PCUT_CUSTOM_MAIN().
 *
 * @warning The callback is called for each test and also for the wrapping
 * invocation.
 */
#define PCUT_MAIN_SET_INIT_HOOK(callback) \
	{ PCUT_MAIN_EXTRA_INIT_HOOK, callback, NULL }

/** Set callback for PCUT pre-initialization.
 *
 * Use from within PCUT_CUSTOM_MAIN().
 * This callback is useful only if you want to manipulate command-line
 * arguments.
 * You probably will not need this.
 *
 * @warning The callback is called for each test and also for the wrapping
 * invocation.
 */
#define PCUT_MAIN_SET_PREINIT_HOOK(callback) \
	{ PCUT_MAIN_EXTRA_PREINIT_HOOK, NULL, callback }


/** Set XML report as default.
 *
 * Use from within PCUT_CUSTOM_MAIN().
 *
 */
#define PCUT_MAIN_SET_XML_REPORT \
	{ PCUT_MAIN_EXTRA_REPORT_XML, NULL, NULL }


/** Insert code to run all tests. */
#define PCUT_CUSTOM_MAIN(...) \
	PCUT_MAIN_WITH_NUMBER(PCUT_ITEM_COUNTER, \
		PCUT_VARG_GET_FIRST(__VA_ARGS__, PCUT_MAIN_EXTRA_SET_LAST), \
		PCUT_VARG_SKIP_FIRST(__VA_ARGS__, PCUT_MAIN_EXTRA_SET_LAST) \
 	)

/**
 * @}
 */

#endif
