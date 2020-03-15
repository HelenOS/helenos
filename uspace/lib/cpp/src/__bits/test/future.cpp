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

#include <__bits/test/mock.hpp>
#include <__bits/test/tests.hpp>
#include <chrono>
#include <exception>
#include <future>
#include <tuple>
#include <utility>

using namespace std::chrono_literals;

namespace
{
    template<class R>
    auto prepare()
    {
        auto res = std::tuple<
            std::promise<R>, std::future<R>,
            std::aux::shared_state<R>*
        >{};
        std::get<0>(res) = std::promise<R>{};
        std::get<1>(res) = std::get<0>(res).get_future();
        std::get<2>(res) = std::get<1>(res).__state();

        return res;
    }
}

namespace std::test
{
    bool future_test::run(bool report)
    {
        report_ = report;
        start();

        test_future();
        test_promise();
        test_future_promise();
        test_async();
        test_packaged_task();
        test_shared_future();

        return end();
    }

    const char* future_test::name()
    {
        return "future";
    }

    void future_test::test_future()
    {
        std::future<int> f1{};
        test("default constructed invalid", !f1.valid());

        std::future<int> f2{new std::aux::shared_state<int>{}};
        test("state constructed valid", f2.valid());

        f1 = std::move(f2);
        test("move assignment source invalid", !f2.valid());
        test("move assignment destination valid", f1.valid());

        std::future<int> f3{std::move(f1)};
        test("move construction source invalid", !f1.valid());
        test("move construction destination valid", f3.valid());
    }

    void future_test::test_promise()
    {
        std::promise<int> p1{};
        test("default constructed promise has state", p1.__state());

        std::promise<int> p2{};
        auto* s1 = p1.__state();
        auto* s2 = p2.__state();
        p2.swap(p1);
        std::swap(s1, s2);

        test_eq("swap switches states pt1", s1, p1.__state());
        test_eq("swap switches states pt2", s2, p2.__state());

        std::promise<int> p3{std::move(p1)};
        test_eq("move construction state moved", s1, p3.__state());
        test_eq("move construction source empty", p1.__state(), nullptr);

        p1 = std::move(p3);
        test_eq("move assignment state move", s1, p1.__state());
        test_eq("move assignment source empty", p3.__state(), nullptr);

        p1.set_value(42);
        test("set_value marks state as ready", s1->is_set());
        test_eq("set_value sets value", s1->get(), 42);
    }

    void future_test::test_future_promise()
    {
        /**
         * Note: As we currently have no exception
         *       propagation support, we do not test
         *       exceptions here. However, the logic there
         *       is basically identical to that of the value
         *       setting.
         */
        auto [p1, f1, s1] = prepare<int>();
        test_eq("refcount in basic case", s1->refs(), 2);

        p1.set_value(1);
        test("simple case valid", f1.valid());
        test_eq("simple case get", f1.get(), 1);

        auto [p2, f2, s2] = prepare<int>();
        std::thread t2{
            [&p2](){
                std::this_thread::sleep_for(20ms);
                p2.set_value(42);
            }
        };

        test_eq("parallel get waits and has correct value", f2.get(), 42);

        auto [p3, f3, s3] = prepare<int>();
        std::thread t3{
            [&p3](){
                std::this_thread::sleep_for(20ms);
                p3.set_value(42);
            }
        };

        f3.wait();
        test("after wait value is set", s3->is_set());
        test_eq("after wait value is correct", s3->get(), 42);

        auto [p4, f4, s4] = prepare<int>();
        std::thread t4{
            [&p4](){
                /* p4.set_value_at_thread_exit(42); */
            }
        };
        std::this_thread::sleep_for(10ms); // Let the value be set inside state.

        /* test("shared state marked as ready at thread exit", s4->is_set()); */
        /* test_eq("value set inside state while in thread", s4->get(), 42); */
        /* test_eq("value set at thread exit", f4.get(), 42); */

        mock::clear();
        std::aux::shared_state<std::test::mock>* s5{};
        {
            std::promise<std::test::mock> p5{};
            s5 = p5.__state();
            test_eq("refcount with just promise", s5->refs(), 1);
            {
                auto f5 = p5.get_future();
                test_eq("refcount after creating future", s5->refs(), 2);
            }
            test_eq("refcount after future is destroyed", s5->refs(), 1);
            test_eq("state not destroyed with future", mock::destructor_calls, 0U);
        }
        test_eq("state destroyed with promise", mock::destructor_calls, 1U);

        mock::clear();
        {
            std::aux::shared_state<std::test::mock>* s6{};
            std::future<std::test::mock> f6{};
            {
                std::promise<std::test::mock> p6{};
                s6 = p6.__state();
                {
                    f6 = p6.get_future();
                    test_eq("move construction only increments refcount once", s6->refs(), 2);
                }
            }
            test_eq("refcount after promise is destroyed", s6->refs(), 1);
            test_eq("state not destroyed with promise", mock::destructor_calls, 0U);
        }
        test_eq("state destroyed with future", mock::destructor_calls, 1U);

        auto [p7, f7, s7] = prepare<int>();
        auto res7 = f7.wait_for(5ms);
        test_eq("wait_for timeout", res7, std::future_status::timeout);

        res7 = f7.wait_until(std::chrono::system_clock::now() + 5ms);
        test_eq("wait_until timeout", res7, std::future_status::timeout);

        std::thread t7{
            [&p7](){
                std::this_thread::sleep_for(5ms);
                p7.set_value(42);
            }
        };
        res7 = f7.wait_for(10ms);
        test_eq("wait_for ready", res7, std::future_status::ready);

        auto [p8, f8, s8] = prepare<int>();
        std::thread t8{
            [&p8](){
                std::this_thread::sleep_for(5ms);
                p8.set_value(42);
            }
        };

        auto res8 = f8.wait_until(std::chrono::system_clock::now() + 10ms);
        test_eq("wait_until ready", res8, std::future_status::ready);

        int x{};
        std::promise<int&> p9{};
        std::future<int&> f9 = p9.get_future();
        p9.set_value(x);
        int& y = f9.get();

        test_eq("reference equal to original", x, y);

        ++x;
        test_eq("equal after modifying original", x, y);

        ++y;
        test_eq("equal after modifying reference", x, y);
    }

    void future_test::test_async()
    {
        auto res1 = std::async(
            [](){
                return 42;
            }
        );
        test_eq("ret async default policy", res1.get(), 42);

        auto res2 = std::async(
            std::launch::deferred, [](){
                return 42;
            }
        );
        test_eq("ret async deferred policy", res2.get(), 42);

        auto res3 = std::async(
            std::launch::async, [](){
                return 42;
            }
        );
        test_eq("ret async async policy", res3.get(), 42);

        int x{};
        auto res4 = std::async(
            [&x](){
                x = 42;
            }
        );

        res4.get();
        test_eq("void async", x, 42);
    }

    void future_test::test_packaged_task()
    {
        std::packaged_task<int(int)> pt1{};
        test("default constructed packaged_task not valid", !pt1.valid());

        pt1 = std::packaged_task<int(int)>{
            [](int x){
                return x + 1;
            }
        };
        test("packaged_task default constructed and move assigned valid", pt1.valid());

        auto f1 = pt1.get_future();
        test("future from valid packaged_task valid", f1.valid());

        pt1(10);
        test_eq("result stored in future correct", f1.get(), 11);

        std::packaged_task<int()> pt2{
            [](){
                return 42;
            }
        };
        auto f2 = pt2.get_future();
        pt2();
        test_eq("no argument packaged_task return value correct", f2.get(), 42);

        pt2.reset();
        test_eq("reset causes refcount decrement", f2.__state()->refs(), 1);

        auto f3 = pt2.get_future();
        pt2();
        test_eq("invocation after reset returns correct value", f3.get(), 42);
        test("reset recreates state", (f2.__state() != f3.__state()));
    }

    void future_test::test_shared_future()
    {
        auto [p1, f1, s1] = prepare<int>();
        auto sf1 = f1.share();

        test("future invalid after share", !f1.valid());
        test_eq("shared state moved on share", sf1.__state(), s1);
        test_eq("no refcount increment on share", s1->refs(), 2);

        {
            auto sf2 = sf1;
            test_eq("refcount increment on copy", s1->refs(), 3);
            test_eq("shared state shared between copies", sf1.__state(), sf2.__state());
        }
        test_eq("refcount decrement when copy gets destroyed", s1->refs(), 2);

        auto sf2 = sf1;
        int res1{}, res2{};
        std::thread t1{
            [&](){
                res1 = sf1.get();
            }
        };
        std::thread t2{
            [&](){
                res2 = sf2.get();
            }
        };

        std::this_thread::sleep_for(20ms);
        p1.set_value(42);
        std::this_thread::sleep_for(20ms);
        test_eq("first result correct", res1, 42);
        test_eq("second result correct", res2, 42);
    }
}
