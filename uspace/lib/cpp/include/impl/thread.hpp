/*
 * Copyright (c) 2017 Jaroslav Jindrak
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

#ifndef LIBCPP_THREAD
#define LIBCPP_THREAD

#include <chrono>
#include <internal/common.hpp>
#include <ostream>

namespace std
{
    extern "C" {
        #include <fibril.h>
        #include <fibril_synch.h>
    }

    /**
     * 30.3.1, class thread:
     */

    class thread
    {
        public:
            class id;

            using native_handle_type = fibril_t*;

            /**
             * 30.3.1.2, thread constructors:
             * 30.3.1.3, thread destructor:
             * 30.3.1.4, thread assignment:
             */

            thread() noexcept;

            ~thread();

            // TODO: check the remark in the standard
            template<class F, class... Args>
            explicit thread(F&& f, Args&&... args)
            {
                // TODO: create std::function out of f and args,
                //       then use fibril_create with that as
                //       the argument and call it?
                id_ = fibril_create(f, nullptr);
                fibril_add_ready(id_);
            }

            thread(const thread&) = delete;
            thread& operator=(const thread&) = delete;

            thread(thread&& other) noexcept;
            thread& operator=(thread&& other) noexcept;

            /**
             * 30.3.1.5, thread members:
             */

            void swap(thread& other) noexcept;

            bool joinable() const noexcept;

            void join();

            void detach();

            id get_id() const noexcept;

            native_handle_type native_handle();

            static unsigned hardware_concurrency() noexcept;

        private:
            fid_t id_;
    };

    void swap(thread& x, thread& y) noexcept;

    /**
     * 30.3.2, namespace this_thread:
     */

    namespace this_thread
    {
        thread::id get_id() noexcept;

        void yield() noexcept;

        template<class Clock, class Duration>
        void sleep_until(const chrono::time_point<Clock, Duration>& abs_time)
        {
            auto now = Clock::now();
            auto usecs = chrono::duration_cast<chrono::duration<typename Duration::rep, micro>>(abs_time - now);

            std::fibril_usleep(usecs.count());
        }

        template<class Rep, class Period>
        void sleep_for(const chrono::duration<Rep, Period>& rel_time)
        {
            if (rel_time <= chrono::duration<Rep, Period>::zero())
                return;

            // TODO: timeouts?
            auto usecs = chrono::duration_cast<chrono::duration<Rep, micro>>(rel_time);
            std::fibril_usleep(usecs.count());
        }
    }

    template<class T>
    struct hash;

    class thread::id
    {
        public:
            id() noexcept
                : id_{}
            { /* DUMMY BODY */ }

            fid_t id_; // For testing atm public.
        private:
            id(fid_t id)
                : id_{id}
            { /* DUMMY BODY */ }

            friend class thread;

            friend bool operator==(thread::id, thread::id) noexcept;
            friend bool operator!=(thread::id, thread::id) noexcept;
            friend bool operator<(thread::id, thread::id) noexcept;
            friend bool operator<=(thread::id, thread::id) noexcept;
            friend bool operator>(thread::id, thread::id) noexcept;
            friend bool operator>=(thread::id, thread::id) noexcept;

            template<class Char, class Traits>
            friend basic_ostream<Char, Traits>& operator<<(
                basic_ostream<Char, Traits>&, thread::id);

            friend struct hash<id>;

            friend id this_thread::get_id() noexcept;
    };

    bool operator==(thread::id lhs, thread::id rhs) noexcept;
    bool operator!=(thread::id lhs, thread::id rhs) noexcept;
    bool operator<(thread::id lhs, thread::id rhs) noexcept;
    bool operator<=(thread::id lhs, thread::id rhs) noexcept;
    bool operator>(thread::id lhs, thread::id rhs) noexcept;
    bool operator>=(thread::id lhs, thread::id rhs) noexcept;

    template<class Char, class Traits>
    basic_ostream<Char, Traits>& operator<<(basic_ostream<Char, Traits>& out, thread::id id)
    {
        out << id.id_;

        return out;
    }

    template<> // TODO: implement
    struct hash<thread::id>;
}

#endif
