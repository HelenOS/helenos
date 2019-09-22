/*
 * Copyright (c) 2019 Jaroslav Jindrak
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

#ifndef LIBCPP_BITS_TEST_TESTS
#define LIBCPP_BITS_TEST_TESTS

#include <__bits/test/test.hpp>
#include <cstdio>
#include <vector>

namespace std::test
{
    class test_set
    {
        public:
            test_set() = default;

            template<class T>
            void add()
            {
                tests_.push_back(new T{});
            }

            bool run(bool report)
            {
                bool res{true};
                unsigned int succeeded{};
                unsigned int failed{};

                for (auto test: tests_)
                {
                    res &= test->run(report);
                    succeeded += test->get_succeeded();
                    failed += test->get_failed();
                }

                if (report)
                {
                    std::printf("\n");
                    if (res)
                        std::printf("[TESTS SUCCEEDED!]");
                    else
                        std::printf("[TESTS FAILED]");
                    std::printf("[%u OK][%u FAIL][%u TOTAL]\n",
                                succeeded, failed, (succeeded + failed));
                }

                return res;
            }

            ~test_set()
            {
                for (auto ptr: tests_)
                    delete ptr;
            }
        private:
            std::vector<test_suite*> tests_{};
    };

    class array_test: public test_suite
    {
        public:
            bool run(bool) override;
            const char* name() override;

            array_test() = default;
            ~array_test() = default;
    };

    class vector_test: public test_suite
    {
        public:
            bool run(bool) override;
            const char* name() override;

            vector_test() = default;
            ~vector_test() = default;

        private:
            void test_construction_and_assignment();
            void test_insert();
            void test_erase();
    };

    class string_test: public test_suite
    {
        public:
            bool run(bool) override;
            const char* name() override;

        private:
            void test_construction_and_assignment();
            void test_append();
            void test_insert();
            void test_erase();
            void test_replace();
            void test_copy();
            void test_find();
            void test_substr();
            void test_compare();
    };

    class bitset_test: public test_suite
    {
        public:
            bool run(bool) override;
            const char* name() override;

        private:
            void test_constructors_and_assignment();
            void test_strings();
            void test_operations();
    };

    class deque_test: public test_suite
    {
        public:
            bool run(bool) override;
            const char* name() override;

        private:
            void test_constructors_and_assignment();
            void test_resizing();
            void test_push_pop();
            void test_operations();
    };

    class tuple_test: public test_suite
    {
        public:
            bool run(bool) override;
            const char* name() override;

        private:
            void test_constructors_and_assignment();
            void test_creation();
            void test_tie_and_structured_bindings();
            void test_tuple_ops();
    };

    class map_test: public test_suite
    {
        public:
            bool run(bool) override;
            const char* name() override;

        private:
            void test_constructors_and_assignment();
            void test_histogram();
            void test_emplace_insert();
            void test_bounds_and_ranges();
            void test_multi();
            void test_reverse_iterators();
            void test_multi_bounds_and_ranges();
    };

    class set_test: public test_suite
    {
        public:
            bool run(bool) override;
            const char* name() override;

        private:
            void test_constructors_and_assignment();
            void test_emplace_insert();
            void test_bounds_and_ranges();
            void test_multi();
            void test_reverse_iterators();
            void test_multi_bounds_and_ranges();
    };

    class unordered_map_test: public test_suite
    {
        public:
            bool run(bool) override;
            const char* name() override;

        private:
            void test_constructors_and_assignment();
            void test_histogram();
            void test_emplace_insert();
            void test_multi();
    };

    class unordered_set_test: public test_suite
    {
        public:
            bool run(bool) override;
            const char* name() override;

        private:
            void test_constructors_and_assignment();
            void test_emplace_insert();
            void test_multi();
    };

    class numeric_test: public test_suite
    {
        public:
            bool run(bool) override;
            const char* name() override;

        private:
            void test_algorithms();
            void test_complex();
    };

    class adaptors_test: public test_suite
    {
        public:
            bool run(bool) override;
            const char* name() override;

        private:
            void test_queue();
            void test_priority_queue();
            void test_stack();
    };

    class memory_test: public test_suite
    {
        public:
            bool run(bool) override;
            const char* name() override;

        private:
            void test_unique_ptr();
            void test_shared_ptr();
            void test_weak_ptr();
            void test_allocators();
            void test_pointers();
    };

    class list_test: public test_suite
    {
        public:
            bool run(bool) override;
            const char* name() override;

        private:
            void test_construction_and_assignment();
            void test_modifiers();
    };

    class ratio_test: public test_suite
    {
        public:
            bool run(bool) override;
            const char* name() override;
    };

    class functional_test: public test_suite
    {
        public:
            bool run(bool) override;
            const char* name() override;
        private:
            void test_reference_wrapper();
            void test_function();
            void test_bind();
    };

    class algorithm_test: public test_suite
    {
        public:
            bool run(bool) override;
            const char* name() override;
        private:
            void test_non_modifying();
            void test_mutating();
    };

    class future_test: public test_suite
    {
        public:
            bool run(bool) override;
            const char* name() override;
        private:
            void test_future();
            void test_promise();
            void test_future_promise();
            void test_async();
            void test_packaged_task();
            void test_shared_future();
    };
}

#endif
