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

#include <__bits/test/mock.hpp>
#include <__bits/test/tests.hpp>
#include <initializer_list>
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
            using value_type         = int;
            using pointer            = char*;
            using const_pointer      = const void*;
            using void_pointer       = bool*;
            using const_void_pointer = volatile bool*;
            using difference_type    = short;
            using size_type          = long;

            using propagate_on_container_copy_assignment = std::true_type;
            using propagate_on_container_move_assignment = std::true_type;
            using propagate_on_container_swap            = std::true_type;
            using is_always_equal                        = std::true_type;
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

        /**
         * First dummy allocator doesn't provide
         * anything except for the mandatory value_type,
         * so we get all the defaults here.
         */
        test(
            "allocator traits default for pointer",
            std::is_same_v<typename dummy_traits1::pointer, int*>
        );
        test(
            "allocator traits default for const_pointer",
            std::is_same_v<typename dummy_traits1::const_pointer, const int*>
        );
        test(
            "allocator traits default for void_pointer",
            std::is_same_v<typename dummy_traits1::void_pointer, void*>
        );
        test(
            "allocator traits default for const_void_pointer",
            std::is_same_v<typename dummy_traits1::const_void_pointer, const void*>
        );
        test(
            "allocator traits default for difference_type",
            std::is_same_v<typename dummy_traits1::difference_type, ptrdiff_t>
        );
        test(
            "allocator traits default for size_type",
            std::is_same_v<typename dummy_traits1::size_type, std::make_unsigned_t<ptrdiff_t>>
        );
        test(
            "allocator traits default for copy propagate",
            std::is_same_v<typename dummy_traits1::propagate_on_container_copy_assignment, std::false_type>
        );
        test(
            "allocator traits default for move propagate",
            std::is_same_v<typename dummy_traits1::propagate_on_container_move_assignment, std::false_type>
        );
        test(
            "allocator traits default for swap propagate",
            std::is_same_v<typename dummy_traits1::propagate_on_container_swap, std::false_type>
        );
        test(
            "allocator traits default for is_always_equal",
            std::is_same_v<typename dummy_traits1::is_always_equal, typename std::is_empty<aux::dummy_allocator1>::type>
        );

        /**
         * Second dummy allocator provides all typedefs, so
         * the the traits just use identity.
         */
        test(
            "allocator traits given pointer",
            std::is_same_v<typename dummy_traits2::pointer, char*>
        );
        test(
            "allocator traits given const_pointer",
            std::is_same_v<typename dummy_traits2::const_pointer, const void*>
        );
        test(
            "allocator traits given void_pointer",
            std::is_same_v<typename dummy_traits2::void_pointer, bool*>
        );
        test(
            "allocator traits given const_void_pointer",
            std::is_same_v<typename dummy_traits2::const_void_pointer, volatile bool*>
        );
        test(
            "allocator traits given difference_type",
            std::is_same_v<typename dummy_traits2::difference_type, short>
        );
        test(
            "allocator traits given size_type",
            std::is_same_v<typename dummy_traits2::size_type, long>
        );
        test(
            "allocator traits given copy propagate",
            std::is_same_v<typename dummy_traits2::propagate_on_container_copy_assignment, std::true_type>
        );
        test(
            "allocator traits given move propagate",
            std::is_same_v<typename dummy_traits2::propagate_on_container_move_assignment, std::true_type>
        );
        test(
            "allocator traits given swap propagate",
            std::is_same_v<typename dummy_traits2::propagate_on_container_swap, std::true_type>
        );
        test(
            "allocator traits given is_always_equal",
            std::is_same_v<typename dummy_traits2::is_always_equal, std::true_type>
        );
    }

    void memory_test::test_pointers()
    {
        using dummy_traits1 = std::pointer_traits<aux::dummy_pointer1>;
        using dummy_traits2 = std::pointer_traits<aux::dummy_pointer2<int, char>>;
        using int_traits    = std::pointer_traits<int*>;

        test(
            "pointer traits pointer pt1",
            std::is_same_v<typename dummy_traits1::pointer, aux::dummy_pointer1>
        );
        test(
            "pointer traits element_type pt1",
            std::is_same_v<typename dummy_traits1::element_type, int>
        );
        test(
            "pointer traits difference_type pt1",
            std::is_same_v<typename dummy_traits1::difference_type, bool>
        );
        test(
            "pointer traits rebind pt1",
            std::is_same_v<typename dummy_traits1::template rebind<long>, unsigned>
        );

        test(
            "pointer traits pointer pt2",
            std::is_same_v<typename dummy_traits2::pointer, aux::dummy_pointer2<int, char>>
        );
        test(
            "pointer traits element_type pt2",
            std::is_same_v<typename dummy_traits2::element_type, signed char>
        );
        test(
            "pointer traits difference_type pt2",
            std::is_same_v<typename dummy_traits2::difference_type, unsigned char>
        );
        test(
            "pointer traits rebind pt2",
            std::is_same_v<typename dummy_traits2::template rebind<long>, aux::dummy_pointer2<long, char>>
        );

        test(
            "pointer traits pointer pt3",
            std::is_same_v<typename int_traits::pointer, int*>
        );
        test(
            "pointer traits element_type pt3",
            std::is_same_v<typename int_traits::element_type, int>
        );
        test(
            "pointer traits difference_type pt3",
            std::is_same_v<typename int_traits::difference_type, ptrdiff_t>
        );
        test(
            "pointer traits rebind pt3",
            std::is_same_v<typename int_traits::rebind<char>, char*>
        );

        int x{10};
        test_eq("pointer_traits<Ptr>::pointer_to", dummy_traits1::pointer_to(x).tag, 10);
        test_eq("pointer_traits<T*>::pointer_to", int_traits::pointer_to(x), &x);
    }
}
