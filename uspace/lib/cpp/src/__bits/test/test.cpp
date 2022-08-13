/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <__bits/test/test.hpp>
#include <cstdio>

namespace std::test
{
    void test_suite::report(bool result, const char* tname)
    {
        if (!report_)
            return;

        if (result)
            std::printf("[%s][%s] ... OK\n", name(), tname);
        else
            std::printf("[%s][%s] ... FAIL\n", name(), tname);
    }

    void test_suite::start()
    {
        if (report_)
            std::printf("\n[TEST START][%s]\n", name());
    }

    bool test_suite::end()
    {
        if (report_)
            std::printf("[TEST END][%s][%u OK][%u FAIL]\n",
                        name(), succeeded_, failed_);
        return ok_;
    }

    unsigned int test_suite::get_failed() const noexcept
    {
        return failed_;
    }

    unsigned int test_suite::get_succeeded() const noexcept
    {
        return succeeded_;
    }
}
