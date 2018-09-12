/*
 * Copyright (c) 2012-2013 Vojtech Horky
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
 *
 * The main control loop of the whole library.
 */

#include "internal.h"
#include "report/report.h"

#pragma warning(push, 0)
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#pragma warning(pop)


/** Current running mode. */
int pcut_run_mode = PCUT_RUN_MODE_FORKING;

/** Empty list to bypass special handling for NULL. */
static pcut_main_extra_t empty_main_extra[] = {
	PCUT_MAIN_EXTRA_SET_LAST
};

/** Helper for iteration over main extras. */
#define FOR_EACH_MAIN_EXTRA(extras, it) \
	for (it = extras; it->type != PCUT_MAIN_EXTRA_LAST; it++)

/** Checks whether the argument is an option followed by a number.
 *
 * @param arg Argument from the user.
 * @param opt Option, including the leading dashes.
 * @param value Where to store the integer value.
 * @return Whether @p arg is @p opt followed by a number.
 */
int pcut_is_arg_with_number(const char *arg, const char *opt, int *value) {
	int opt_len = pcut_str_size(opt);
	if (! pcut_str_start_equals(arg, opt, opt_len)) {
		return 0;
	}
	*value = pcut_str_to_int(arg + opt_len);
	return 1;
}


/** Find item by its id.
 *
 * @param first List to search.
 * @param id Id to find.
 * @return The item with given id.
 * @retval NULL No item with such id exists in the list.
 */
static pcut_item_t *pcut_find_by_id(pcut_item_t *first, int id) {
	pcut_item_t *it = pcut_get_real(first);
	while (it != NULL) {
		if (it->id == id) {
			return it;
		}
		it = pcut_get_real_next(it);
	}
	return NULL;
}

/** Run the whole test suite.
 *
 * @param suite Suite to run.
 * @param last Pointer to first item after this suite is stored here.
 * @param prog_path Path to the current binary (used in forked mode).
 * @return Error code.
 */
static int run_suite(pcut_item_t *suite, pcut_item_t **last, const char *prog_path) {
	int is_first_test = 1;
	int total_count = 0;
	int ret_code = PCUT_OUTCOME_PASS;
	int ret_code_tmp;

	pcut_item_t *it = pcut_get_real_next(suite);
	if ((it == NULL) || (it->kind == PCUT_KIND_TESTSUITE)) {
		goto leave_no_print;
	}

	for (; it != NULL; it = pcut_get_real_next(it)) {
		if (it->kind == PCUT_KIND_TESTSUITE) {
			goto leave_ok;
		}
		if (it->kind != PCUT_KIND_TEST) {
			continue;
		}

		if (is_first_test) {
			pcut_report_suite_start(suite);
			is_first_test = 0;
		}

		if (pcut_run_mode == PCUT_RUN_MODE_FORKING) {
			ret_code_tmp = pcut_run_test_forking(prog_path, it);
		} else {
			ret_code_tmp = pcut_run_test_single(it);
		}

		/*
		 * Override final return code in case of failure.
		 *
		 * In this case we suppress any special error codes as
		 * to the outside, there was a failure.
		 */
		if (ret_code_tmp != PCUT_OUTCOME_PASS) {
			ret_code = PCUT_OUTCOME_FAIL;
		}

		total_count++;
	}

leave_ok:
	if (total_count > 0) {
		pcut_report_suite_done(suite);
	}

leave_no_print:
	if (last != NULL) {
		*last = it;
	}

	return ret_code;
}

/** Add direct pointers to set-up/tear-down functions to a suites.
 *
 * At start-up, set-up and tear-down functions are scattered in the
 * list as siblings of suites and tests.
 * This puts them into the structure describing the suite itself.
 *
 * @param first First item of the list.
 */
static void set_setup_teardown_callbacks(pcut_item_t *first) {
	pcut_item_t *active_suite = NULL;
	pcut_item_t *it;
	for (it = first; it != NULL; it = pcut_get_real_next(it)) {
		if (it->kind == PCUT_KIND_TESTSUITE) {
			active_suite = it;
		} else if (it->kind == PCUT_KIND_SETUP) {
			if (active_suite != NULL) {
				active_suite->setup_func = it->setup_func;
			}
			it->kind = PCUT_KIND_SKIP;
		} else if (it->kind == PCUT_KIND_TEARDOWN) {
			if (active_suite != NULL) {
				active_suite->teardown_func = it->teardown_func;
			}
			it->kind = PCUT_KIND_SKIP;
		} else {
			/* Not interesting right now. */
		}
	}
}

/** The main function of PCUT.
 *
 * This function is expected to be called as the only function in
 * normal main().
 *
 * @param last Pointer to the last item defined by PCUT_TEST macros.
 * @param argc Original argc of the program.
 * @param argv Original argv of the program.
 * @return Program exit code.
 */
int pcut_main(pcut_item_t *last, int argc, char *argv[]) {
	pcut_item_t *items = pcut_fix_list_get_real_head(last);
	pcut_item_t *it;
	pcut_main_extra_t *main_extras = last->main_extras;
	pcut_main_extra_t *main_extras_it;

	int run_only_suite = -1;
	int run_only_test = -1;

	int rc, rc_tmp;

	if (main_extras == NULL) {
		main_extras = empty_main_extra;
	}

	pcut_report_register_handler(&pcut_report_tap);

	FOR_EACH_MAIN_EXTRA(main_extras, main_extras_it) {
		if (main_extras_it->type == PCUT_MAIN_EXTRA_REPORT_XML) {
			pcut_report_register_handler(&pcut_report_xml);
		}
		if (main_extras_it->type == PCUT_MAIN_EXTRA_PREINIT_HOOK) {
			main_extras_it->preinit_hook(&argc, &argv);
		}
	}

	if (argc > 1) {
		int i;
		for (i = 1; i < argc; i++) {
			pcut_is_arg_with_number(argv[i], "-s", &run_only_suite);
			pcut_is_arg_with_number(argv[i], "-t", &run_only_test);
			if (pcut_str_equals(argv[i], "-l")) {
				pcut_print_tests(items);
				return PCUT_OUTCOME_PASS;
			}
			if (pcut_str_equals(argv[i], "-x")) {
				pcut_report_register_handler(&pcut_report_xml);
			}
#ifndef PCUT_NO_LONG_JUMP
			if (pcut_str_equals(argv[i], "-u")) {
				pcut_run_mode = PCUT_RUN_MODE_SINGLE;
			}
#endif
		}
	}

	setvbuf(stdout, NULL, _IONBF, 0);
	set_setup_teardown_callbacks(items);

	FOR_EACH_MAIN_EXTRA(main_extras, main_extras_it) {
		if (main_extras_it->type == PCUT_MAIN_EXTRA_INIT_HOOK) {
			main_extras_it->init_hook();
		}
	}

	PCUT_DEBUG("run_only_suite = %d   run_only_test = %d", run_only_suite, run_only_test);

	if ((run_only_suite >= 0) && (run_only_test >= 0)) {
		printf("Specify either -s or -t!\n");
		return PCUT_OUTCOME_BAD_INVOCATION;
	}

	if (run_only_suite > 0) {
		pcut_item_t *suite = pcut_find_by_id(items, run_only_suite);
		if (suite == NULL) {
			printf("Suite not found, aborting!\n");
			return PCUT_OUTCOME_BAD_INVOCATION;
		}
		if (suite->kind != PCUT_KIND_TESTSUITE) {
			printf("Invalid suite id!\n");
			return PCUT_OUTCOME_BAD_INVOCATION;
		}

		run_suite(suite, NULL, argv[0]);
		return PCUT_OUTCOME_PASS;
	}

	if (run_only_test > 0) {
		pcut_item_t *test = pcut_find_by_id(items, run_only_test);
		if (test == NULL) {
			printf("Test not found, aborting!\n");
			return PCUT_OUTCOME_BAD_INVOCATION;
		}
		if (test->kind != PCUT_KIND_TEST) {
			printf("Invalid test id!\n");
			return PCUT_OUTCOME_BAD_INVOCATION;
		}

		if (pcut_run_mode == PCUT_RUN_MODE_SINGLE) {
			rc = pcut_run_test_single(test);
		} else {
			rc = pcut_run_test_forked(test);
		}

		return rc;
	}

	/* Otherwise, run the whole thing. */
	pcut_report_init(items);

	rc = PCUT_OUTCOME_PASS;

	it = items;
	while (it != NULL) {
		if (it->kind == PCUT_KIND_TESTSUITE) {
			pcut_item_t *tmp;
			rc_tmp = run_suite(it, &tmp, argv[0]);
			if (rc_tmp != PCUT_OUTCOME_PASS) {
				rc = rc_tmp;
			}
			it = tmp;
		} else {
			it = pcut_get_real_next(it);
		}
	}

	pcut_report_done();

	return rc;
}
