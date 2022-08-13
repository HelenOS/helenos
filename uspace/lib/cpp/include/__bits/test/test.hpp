/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_TEST
#define LIBCPP_BITS_TEST

#include <utility>
#include <iterator>
#include <type_traits>

namespace std::test
{
    class test_suite
    {
        public:
            virtual bool run(bool) = 0;
            virtual const char* name() = 0;

            virtual ~test_suite() = default;

            unsigned int get_failed() const noexcept;

            unsigned int get_succeeded() const noexcept;

        protected:
            void report(bool, const char*);
            void start();
            bool end();

            unsigned int failed_{};
            unsigned int succeeded_{};
            bool ok_{true};
            bool report_{true};

            void test(const char* tname, bool expr)
            {
                test_eq(tname, expr, true);
            }

            template<class... Args>
            void test_eq(const char* tname, Args&&... args)
            {
                if (!assert_eq(std::forward<Args>(args)...))
                {
                    report(false, tname);
                    ++failed_;
                    ok_ = false;
                }
                else
                {
                    report(true, tname);
                    ++succeeded_;
                }
            }

            template<class T, class U>
            bool assert_eq(const T& lhs, const U& rhs)
            {
                return lhs == rhs;
            }

            template<class Iterator1, class Iterator2>
            bool assert_eq(Iterator1 first1, Iterator1 last1,
                           Iterator2 first2, Iterator2 last2)
            {
                auto len1 = std::distance(first1, last1);
                auto len2 = std::distance(first2, last2);

                using common_t = std::common_type_t<decltype(len1), decltype(len2)>;
                auto len1_common = static_cast<common_t>(len1);
                auto len2_common = static_cast<common_t>(len2);

                if (len1_common != len2_common)
                    return false;

                while (first1 != last1)
                {
                    if (*first1++ != *first2++)
                        return false;
                }

                return true;
            }

            template<class Iterator, class Container>
            void test_contains(const char* tname, Iterator first,
                               Iterator last, const Container& cont)
            {
                if (!assert_contains(first, last, cont))
                {
                    report(false, tname);
                    ++failed_;
                    ok_ = false;
                }
                else
                {
                    report(true, tname);
                    ++succeeded_;
                }
            }

            template<class Iterator1, class Iterator2, class Container>
            void test_contains_multi(const char* tname, Iterator1 first1,
                               Iterator1 last1, Iterator2 first2,
                               const Container& cont)
            {
                if (!assert_contains_multi(first1, last1, first2, cont))
                {
                    report(false, tname);
                    ++failed_;
                    ok_ = false;
                }
                else
                {
                    report(true, tname);
                    ++succeeded_;
                }
            }

            template<class Iterator, class Container>
            bool assert_contains(Iterator first, Iterator last, const Container& cont)
            {
                while (first != last)
                {
                    if (cont.find(*first++) == cont.end())
                        return false;
                }

                return true;
            }

            template<class Iterator1, class Iterator2, class Container>
            bool assert_contains_multi(Iterator1 first1, Iterator1 last1,
                                       Iterator2 first2, const Container& cont)
            {
                while (first1 != last1)
                {
                    if (cont.count(*first1++) != *first2++)
                        return false;
                }

                return true;
            }
    };
}

#endif
