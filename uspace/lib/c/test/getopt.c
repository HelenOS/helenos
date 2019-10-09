/*
 * Copyright (c) 2019 Matthieu Riolo
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

#include <pcut/pcut.h>
#include <getopt.h>
#include <stdio.h>

PCUT_INIT;

PCUT_TEST_SUITE(getopt);

PCUT_TEST(getopt_param_flag)
{
	int argc = 4;
	const char *argv[] = {
		"get_opt_test",
		"-f",
		"-p",
		"param",
	};

	const char *options = "fp:";

	int ret;
	optreset = 1;
	opterr = 0;

	ret = getopt(argc, (char *const *)argv, options);

	PCUT_ASSERT_INT_EQUALS('f', ret);
	PCUT_ASSERT_INT_EQUALS(2, optind);

	ret = getopt(argc, (char *const *)argv, options);

	PCUT_ASSERT_INT_EQUALS('p', ret);
	PCUT_ASSERT_INT_EQUALS(4, optind);
	PCUT_ASSERT_STR_EQUALS("param", optarg);

	ret = getopt(argc, (char *const *)argv, options);
	PCUT_ASSERT_INT_EQUALS(-1, ret);
}

PCUT_TEST(getopt_concat_flags)
{
	int argc = 2;
	const char *argv[] = {
		"get_opt_test",
		"-fda",
	};

	const char *options = "afd";

	int ret;
	optreset = 1;
	opterr = 0;

	ret = getopt(argc, (char *const *)argv, options);

	PCUT_ASSERT_INT_EQUALS('f', ret);
	PCUT_ASSERT_INT_EQUALS(1, optind);

	ret = getopt(argc, (char *const *)argv, options);

	PCUT_ASSERT_INT_EQUALS('d', ret);
	PCUT_ASSERT_INT_EQUALS(1, optind);

	ret = getopt(argc, (char *const *)argv, options);

	PCUT_ASSERT_INT_EQUALS('a', ret);
	PCUT_ASSERT_INT_EQUALS(2, optind);

	ret = getopt(argc, (char *const *)argv, options);
	PCUT_ASSERT_INT_EQUALS(-1, ret);
}

PCUT_TEST(getopt_concat_flag_param)
{
	int argc = 3;
	const char *argv[] = {
		"get_opt_test",
		"-fp",
		"param"
	};

	const char *options = "fp:";

	int ret;
	optreset = 1;
	opterr = 0;

	ret = getopt(argc, (char *const *)argv, options);

	PCUT_ASSERT_INT_EQUALS('f', ret);
	PCUT_ASSERT_INT_EQUALS(1, optind);

	ret = getopt(argc, (char *const *)argv, options);

	PCUT_ASSERT_INT_EQUALS('p', ret);
	PCUT_ASSERT_INT_EQUALS(3, optind);
	PCUT_ASSERT_STR_EQUALS("param", optarg);

	ret = getopt(argc, (char *const *)argv, options);
	PCUT_ASSERT_INT_EQUALS(-1, ret);
}

PCUT_TEST(getopt_missing_param1)
{
	int argc = 2;
	const char *argv[] = {
		"get_opt_test",
		"-p",
	};

	const char *options = "p:";

	int ret;
	optreset = 1;
	opterr = 0;

	ret = getopt(argc, (char *const *)argv, options);

	PCUT_ASSERT_INT_EQUALS('?', ret);
	PCUT_ASSERT_INT_EQUALS('p', optopt);
	PCUT_ASSERT_INT_EQUALS(2, optind);

	ret = getopt(argc, (char *const *)argv, options);
	PCUT_ASSERT_INT_EQUALS(-1, ret);
}

PCUT_TEST(getopt_missing_param2)
{
	int argc = 2;
	const char *argv[] = {
		"get_opt_test",
		"-p",
	};

	const char *options = ":p:";

	int ret;
	optreset = 1;
	opterr = 0;

	ret = getopt(argc, (char *const *)argv, options);

	PCUT_ASSERT_INT_EQUALS(':', ret);
	PCUT_ASSERT_INT_EQUALS('p', optopt);
	PCUT_ASSERT_INT_EQUALS(2, optind);

	ret = getopt(argc, (char *const *)argv, options);
	PCUT_ASSERT_INT_EQUALS(-1, ret);
}

PCUT_TEST(getopt_illegal_option)
{
	int argc = 2;
	const char *argv[] = {
		"get_opt_test",
		"-p",
	};

	const char *options = "a";

	int ret;
	optreset = 1;
	opterr = 0;

	ret = getopt(argc, (char *const *)argv, options);

	PCUT_ASSERT_INT_EQUALS('?', ret);
	PCUT_ASSERT_INT_EQUALS('p', optopt);
	PCUT_ASSERT_INT_EQUALS(2, optind);

	ret = getopt(argc, (char *const *)argv, options);
	PCUT_ASSERT_INT_EQUALS(-1, ret);

	options = ":a";
	optreset = 1;
	ret = getopt(argc, (char *const *)argv, options);

	PCUT_ASSERT_INT_EQUALS(-1, ret);
	PCUT_ASSERT_INT_EQUALS('p', optopt);
	PCUT_ASSERT_INT_EQUALS(2, optind);

	ret = getopt(argc, (char *const *)argv, options);
	PCUT_ASSERT_INT_EQUALS(-1, ret);
}

PCUT_TEST(getopt_flag_with_param)
{
	int argc = 4;
	const char *argv[] = {
		"get_opt_test",
		"-f",
		"param",
		"-d"
	};

	const char *options = "fd";

	int ret;
	optreset = 1;
	opterr = 0;

	ret = getopt(argc, (char *const *)argv, options);

	/* getopt() would print a error message but thx to opterror=0 it doesnt */
	PCUT_ASSERT_INT_EQUALS('f', ret);
	PCUT_ASSERT_INT_EQUALS(2, optind);

	ret = getopt(argc, (char *const *)argv, options);

	PCUT_ASSERT_INT_EQUALS('d', ret);
	PCUT_ASSERT_INT_EQUALS(4, optind);

	ret = getopt(argc, (char *const *)argv, options);
	PCUT_ASSERT_INT_EQUALS(-1, ret);
}

PCUT_TEST(getopt_case_sensitive)
{
	int argc = 3;
	const char *argv[] = {
		"get_opt_test",
		"-F",
		"-f"
	};

	const char *options = "fF";

	int ret;
	optreset = 1;
	opterr = 0;

	ret = getopt(argc, (char *const *)argv, options);

	PCUT_ASSERT_INT_EQUALS('F', ret);
	PCUT_ASSERT_INT_EQUALS(2, optind);

	ret = getopt(argc, (char *const *)argv, options);

	PCUT_ASSERT_INT_EQUALS('f', ret);
	PCUT_ASSERT_INT_EQUALS(3, optind);

	ret = getopt(argc, (char *const *)argv, options);
	PCUT_ASSERT_INT_EQUALS(-1, ret);
}

PCUT_TEST(getopt_optional_param)
{
	int argc = 4;
	const char *argv[] = {
		"get_opt_test",
		"-f",
		"-pparam"
	};

	const char *options = "f::p::";

	int ret;
	optreset = 1;
	opterr = 0;

	ret = getopt(argc, (char *const *)argv, options);

	PCUT_ASSERT_INT_EQUALS('f', ret);
	PCUT_ASSERT_INT_EQUALS(2, optind);
	PCUT_ASSERT_NULL(optarg);

	ret = getopt(argc, (char *const *)argv, options);

	PCUT_ASSERT_INT_EQUALS('p', ret);
	PCUT_ASSERT_INT_EQUALS(3, optind);
	PCUT_ASSERT_STR_EQUALS("param", optarg);

	ret = getopt(argc, (char *const *)argv, options);
	PCUT_ASSERT_INT_EQUALS(-1, ret);
}

PCUT_TEST(getopt_special_option)
{
	int argc = 4;
	const char *argv[] = {
		"get_opt_test",
		"-f",
		"--",
		"-p"
	};

	const char *options = "fp";

	int ret;
	optreset = 1;
	opterr = 0;

	ret = getopt(argc, (char *const *)argv, options);

	PCUT_ASSERT_INT_EQUALS('f', ret);
	PCUT_ASSERT_INT_EQUALS(2, optind);

	ret = getopt(argc, (char *const *)argv, options);
	PCUT_ASSERT_INT_EQUALS(-1, ret);
}

PCUT_TEST(getopt_gnu_plus)
{
	int argc = 4;
	const char *argv[] = {
		"get_opt_test",
		"-f",
		"break",
		"-p"
	};

	const char *options = "+fp";

	int ret;
	optreset = 1;
	opterr = 0;

	ret = getopt(argc, (char *const *)argv, options);

	PCUT_ASSERT_INT_EQUALS('f', ret);
	PCUT_ASSERT_INT_EQUALS(2, optind);

	ret = getopt(argc, (char *const *)argv, options);
	PCUT_ASSERT_INT_EQUALS(-1, ret);
}

PCUT_TEST(getopt_gnu_minus)
{
	int argc = 4;
	const char *argv[] = {
		"get_opt_test",
		"-f",
		"break",
		"-p"
	};

	const char *options = "-fp";

	int ret;
	optreset = 1;
	opterr = 0;

	ret = getopt(argc, (char *const *)argv, options);

	PCUT_ASSERT_INT_EQUALS('f', ret);
	PCUT_ASSERT_INT_EQUALS(2, optind);

	ret = getopt(argc, (char *const *)argv, options);

	PCUT_ASSERT_INT_EQUALS(1, ret);
	PCUT_ASSERT_INT_EQUALS(3, optind);
	PCUT_ASSERT_STR_EQUALS("break", optarg);

	ret = getopt(argc, (char *const *)argv, options);

	PCUT_ASSERT_INT_EQUALS('p', ret);
	PCUT_ASSERT_INT_EQUALS(4, optind);

	ret = getopt(argc, (char *const *)argv, options);
	PCUT_ASSERT_INT_EQUALS(-1, ret);
}

PCUT_TEST(getopt_long_flag_param)
{
	int argc = 4;
	const char *argv[] = {
		"get_opt_test",
		"--flag",
		"--parameter",
		"param"
	};

	const char *options = "fp:";

	const struct option long_options[] = {
		{ "flag", no_argument, NULL, 'f' },
		{ "parameter", required_argument, NULL, 'p' },
	};

	int ret;
	int idx;
	optreset = 1;
	opterr = 0;

	ret = getopt_long(argc, (char *const *)argv, options, long_options, &idx);
	PCUT_ASSERT_INT_EQUALS('f', ret);
	PCUT_ASSERT_INT_EQUALS(2, optind);
	PCUT_ASSERT_INT_EQUALS(0, idx);

	ret = getopt_long(argc, (char *const *)argv, options, long_options, &idx);
	PCUT_ASSERT_INT_EQUALS('p', ret);
	PCUT_ASSERT_INT_EQUALS(4, optind);
	PCUT_ASSERT_INT_EQUALS(1, idx);
	PCUT_ASSERT_STR_EQUALS("param", optarg);

	ret = getopt_long(argc, (char *const *)argv, options, long_options, &idx);
	PCUT_ASSERT_INT_EQUALS(-1, ret);
}

PCUT_TEST(getopt_long_alt_param)
{
	int argc = 3;
	const char *argv[] = {
		"get_opt_test",
		"--flag=\"param param\"",
		"--parameter=param",
	};

	const char *options = "f:p:";

	const struct option long_options[] = {
		{ "flag", required_argument, NULL, 'f' },
		{ "parameter", required_argument, NULL, 'p' },
		{ 0, 0, 0, 0 }
	};

	int ret;
	int idx;
	optreset = 1;
	opterr = 0;

	ret = getopt_long(argc, (char *const *)argv, options, long_options, &idx);
	PCUT_ASSERT_INT_EQUALS('f', ret);
	PCUT_ASSERT_INT_EQUALS(2, optind);
	PCUT_ASSERT_INT_EQUALS(0, idx);
	PCUT_ASSERT_STR_EQUALS("\"param param\"", optarg);

	ret = getopt_long(argc, (char *const *)argv, options, long_options, &idx);
	PCUT_ASSERT_INT_EQUALS('p', ret);
	PCUT_ASSERT_INT_EQUALS(3, optind);
	PCUT_ASSERT_INT_EQUALS(1, idx);
	PCUT_ASSERT_STR_EQUALS("param", optarg);

	ret = getopt_long(argc, (char *const *)argv, options, long_options, &idx);
	PCUT_ASSERT_INT_EQUALS(-1, ret);
}

PCUT_TEST(getopt_long_optional_param)
{
	int argc = 3;
	const char *argv[] = {
		"get_opt_test",
		"--flag=param",
		"--parameter",
	};

	const char *options = "f::p::";

	const struct option long_options[] = {
		{ "flag", optional_argument, NULL, 'f' },
		{ "parameter", optional_argument, NULL, 'p' },
		{ 0, 0, 0, 0 }
	};

	int ret;
	int idx;
	optreset = 1;
	opterr = 0;

	ret = getopt_long(argc, (char *const *)argv, options, long_options, &idx);
	PCUT_ASSERT_INT_EQUALS('f', ret);
	PCUT_ASSERT_INT_EQUALS(2, optind);
	PCUT_ASSERT_INT_EQUALS(0, idx);
	PCUT_ASSERT_STR_EQUALS("param", optarg);

	ret = getopt_long(argc, (char *const *)argv, options, long_options, &idx);
	PCUT_ASSERT_INT_EQUALS('p', ret);
	PCUT_ASSERT_INT_EQUALS(3, optind);
	PCUT_ASSERT_INT_EQUALS(1, idx);
	PCUT_ASSERT_NULL(optarg);

	ret = getopt_long(argc, (char *const *)argv, options, long_options, &idx);
	PCUT_ASSERT_INT_EQUALS(-1, ret);
}

PCUT_TEST(getopt_long_illegal_option)
{
	int argc = 3;
	const char *argv[] = {
		"get_opt_test",
		"--param",
		"param",
	};

	const char *options = "f::";

	const struct option long_options[] = {
		{ "cflag", required_argument, NULL, 'c' },
		{ "flag", required_argument, NULL, 'f' },
		{ 0, 0, 0, 0 }
	};

	int ret;
	int idx;
	optreset = 1;
	opterr = 0;

	ret = getopt_long(argc, (char *const *)argv, options, long_options, &idx);
	PCUT_ASSERT_INT_EQUALS('?', ret);
	PCUT_ASSERT_INT_EQUALS(2, optind);
	PCUT_ASSERT_INT_EQUALS(0, idx);
	PCUT_ASSERT_INT_EQUALS(0, optopt);

	ret = getopt_long(argc, (char *const *)argv, options, long_options, &idx);
	PCUT_ASSERT_INT_EQUALS(-1, ret);
}

PCUT_TEST(getopt_long_ambiguous_param)
{
	int argc = 3;
	const char *argv[] = {
		"get_opt_test",
		"--flag",
		"param",
	};

	const char *options = "f::";

	const struct option long_options[] = {
		{ "flag1", optional_argument, NULL, 'f' },
		{ "flag2", required_argument, NULL, 'f' },
		{ 0, 0, 0, 0 }
	};

	int ret;
	int idx;
	optreset = 1;
	opterr = 0;

	ret = getopt_long(argc, (char *const *)argv, options, long_options, &idx);
	PCUT_ASSERT_INT_EQUALS('?', ret);
	PCUT_ASSERT_INT_EQUALS(2, optind);
	PCUT_ASSERT_INT_EQUALS(0, idx);
	PCUT_ASSERT_INT_EQUALS(0, optopt);

	ret = getopt_long(argc, (char *const *)argv, options, long_options, &idx);
	PCUT_ASSERT_INT_EQUALS(-1, ret);
}

PCUT_EXPORT(getopt);
