/*
 * Copyright (c) 2018 Jaroslav Jindrak
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

#include <initializer_list>
#include <internal/test/mock.hpp>
#include <internal/test/tests.hpp>
#include <memory>
#include <type_traits>
#include <utility>

namespace std::test
{
    namespace aux
    {
        struct dummy_pointer1
        {
            using element_type    = int;
            using difference_type = bool;

            template<class U>
            using rebind = unsigned;

            int tag{};

            static dummy_pointer1 pointer_to(element_type& x)
            {
                dummy_pointer1 res;
                res.tag = x;
                return res;
            }
        };

        template<class T, class... Args>
        struct dummy_pointer2
        {
            using element_type    = signed char;
            using difference_type = unsigned char;
        };

        struct dummy_allocator1
        {
            using value_type = int;
        };

        struct dummy_allocator2
        {
            using value_type    = int;
            using pointer       = char*;
            using const_pointer = const void*;
        };
    }

    bool memory_test::run(bool report)
    {
        report_ = report;
        start();

        test_unique_ptr();
        test_shared_ptr();
        test_weak_ptr();
        test_allocators();
        test_pointers();

        return end();
    }

    const char* memory_test::name()
    {
        return "memory";
    }

    void memory_test::test_unique_ptr()
    {
        mock::clear();
        {
            auto ptr = std::make_unique<mock>();
            test_eq("unique_ptr get() when non-null", (ptr.get() != nullptr), true);
            test_eq("unique_ptr operator bool when non-null", (bool)ptr, true);
        }
        test_eq("unique_ptr make_unique", mock::constructor_calls, 1U);
        test_eq("unique_ptr out of scope", mock::destructor_calls, 1U);

        mock::clear();
        {
            auto ptr = std::make_unique<mock>();
            delete ptr.release();
        }
        test_eq("unique_ptr release", mock::destructor_calls, 1U);

        mock::clear();
        {
            auto ptr = std::make_unique<mock>();
            ptr.reset(new mock{});
        }
        test_eq("unique_ptr reset", mock::destructor_calls, 2U);

        mock::clear();
        {
            std::unique_ptr<mock> ptr1{};
            test_eq("unique_ptr get() when null", ptr1.get(), nullptr);
            test_eq("unique_ptr operator bool when null", (bool)ptr1, false);
            {
                auto ptr2 = std::make_unique<mock>();
                ptr1 = std::move(ptr2);
            }
            test_eq("unique_ptr move pt1", mock::destructor_calls, 0U);
        }
        test_eq("unique_ptr move pt2", mock::destructor_calls, 1U);

        mock::clear();
        {
            auto ptr = std::make_unique<mock[]>(10U);
            test_eq("unique_ptr make_unique array version", mock::constructor_calls, 10U);

            new(&ptr[5]) mock{};
            test_eq("placement new into the array", mock::constructor_calls, 11U);
            test_eq("original not destroyed during placement new", mock::destructor_calls, 0U);
        }
        test_eq("unique_ptr array out of scope", mock::destructor_calls, 10U);
    }

    void memory_test::test_shared_ptr()
    {
        mock::clear();
        {
            auto ptr1 = std::make_shared<mock>();
            test_eq("shared_ptr make_shared", mock::constructor_calls, 1U);
            test_eq("shared_ptr unique", ptr1.unique(), true);
            {
                auto ptr2 = ptr1;
                test_eq("shared_ptr copy pt1", ptr1.use_count(), 2L);
                test_eq("shared_ptr copy pt2", ptr2.use_count(), 2L);
                test_eq("shared_ptr copy no constructor call", mock::copy_constructor_calls, 0U);
                test_eq("shared_ptr not unique", ptr1.unique(), false);

                auto ptr3 = std::move(ptr2);
                test_eq("shared_ptr move pt1", ptr1.use_count(), 2L);
                test_eq("shared_ptr move pt2", ptr3.use_count(), 2L);
                test_eq("shared_ptr move pt3", ptr2.use_count(), 0L);

                test_eq("shared_ptr move origin empty", (bool)ptr2, false);
            }
            test_eq("shared_ptr copy out of scope", mock::destructor_calls, 0U);
        }
        test_eq("shared_ptr original out of scope", mock::destructor_calls, 1U);
    }

    void memory_test::test_weak_ptr()
    {
        mock::clear();
        {
            std::weak_ptr<mock> wptr1{};
            {
                auto ptr1 = std::make_shared<mock>();
                wptr1 = ptr1;
                {
                    std::weak_ptr<mock> wptr2 = ptr1;
                    test_eq("weak_ptr shares use count", wptr2.use_count(), 1L);
                    test_eq("weak_ptr not expired", wptr2.expired(), false);

                    auto ptr2 = wptr2.lock();
                    test_eq("locked ptr increases use count", ptr1.use_count(), 2L);
                }
            }
            test_eq("weak_ptr expired after all shared_ptrs die", wptr1.expired(), true);
            test_eq("shared object destroyed while weak_ptr exists", mock::destructor_calls, 1U);
        }
    }

    void memory_test::test_allocators()
    {
        using dummy_traits1 = std::allocator_traits<aux::dummy_allocator1>;
        using dummy_traits2 = std::allocator_traits<aux::dummy_allocator2>;

        /* static_assert(std::is_same_v<typename dummy_traits1::pointer, int*>); */
        /* static_assert(std::is_same_v<typename dummy_traits2::pointer, char*>); */
    }

    void memory_test::test_pointers()
    {
        using dummy_traits1 = std::pointer_traits<aux::dummy_pointer1>;
        using dummy_traits2 = std::pointer_traits<aux::dummy_pointer2<int, char>>;
        using int_traits    = std::pointer_traits<int*>;

        static_assert(std::is_same_v<typename dummy_traits1::pointer, aux::dummy_pointer1>);
        static_assert(std::is_same_v<typename dummy_traits1::element_type, int>);
        static_assert(std::is_same_v<typename dummy_traits1::difference_type, bool>);
        static_assert(std::is_same_v<typename dummy_traits1::template rebind<long>, unsigned>);

        int x{10};
        test_eq("pointer_traits<Ptr>::pointer_to", dummy_traits1::pointer_to(x).tag, 10);

        static_assert(std::is_same_v<typename dummy_traits2::pointer, aux::dummy_pointer2<int, char>>);
        static_assert(std::is_same_v<typename dummy_traits2::element_type, signed char>);
        static_assert(std::is_same_v<typename dummy_traits2::difference_type, unsigned char>);

        static_assert(std::is_same_v<typename int_traits::pointer, int*>);
        static_assert(std::is_same_v<typename int_traits::element_type, int>);
        static_assert(std::is_same_v<typename int_traits::difference_type, ptrdiff_t>);
        static_assert(std::is_same_v<typename int_traits::rebind<char>, char*>);
    }
}
