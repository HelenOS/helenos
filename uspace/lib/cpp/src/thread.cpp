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

#include <cstdlib>
#include <thread>
#include <utility>

namespace std
{
    thread::thread() noexcept
        : id_{}
    { /* DUMMY BODY */ }

    thread::~thread()
    {
        // TODO: Change this to std::terminate when implemented.
        if (joinable())
        {
            if (joinable_wrapper_)
            {
                joinable_wrapper_->join();
                delete joinable_wrapper_;
            }

            // TODO: this crashes :(
            /* fibril_teardown((fibril_t*)id_, false); */
            /* std::abort(); */
        }
    }

    thread::thread(thread&& other) noexcept
        : id_{other.id_}
    {
        other.id_ = fid_t{};
    }

    thread& thread::operator=(thread&& other) noexcept
    {
        id_ = other.id_;
        other.id_ = fid_t{};

        return *this;
    }

    void thread::swap(thread& other) noexcept
    {
        std::swap(id_, other.id_);
    }

    bool thread::joinable() const noexcept
    {
        return id_ != fid_t{};
    }

    void thread::join()
    {
        if (joinable_wrapper_)
            joinable_wrapper_->join();
    }

    void thread::detach()
    {
        id_ = fid_t{};

        if (joinable_wrapper_)
        {
            joinable_wrapper_->detach();
            joinable_wrapper_ = nullptr;
        }
    }

    thread::id thread::get_id() const noexcept
    {
        return id{id_};
    }

    thread::native_handle_type thread::native_handle()
    {
        /**
         * For fibrils the fid_t returned from fibril_create
         * is a fibril_t* casted to fid_t, native handles
         * are implementation defined so we just recast back.
         */
        return (native_handle_type)id_;
    }

    unsigned thread::hardware_concurrency() noexcept
    {
        // TODO:
        return 0;
    }

    void swap(thread& x, thread& y) noexcept
    {
        x.swap(y);
    }

    namespace this_thread
    {
        thread::id get_id() noexcept
        {
            return thread::id{fibril_get_id()};
        }

        void yield() noexcept
        {
            fibril_yield();
        }
    }

    bool operator==(thread::id lhs, thread::id rhs) noexcept
    {
        return lhs.id_ == rhs.id_;
    }

    bool operator!=(thread::id lhs, thread::id rhs) noexcept
    {
        return !(lhs == rhs);
    }

    bool operator<(thread::id lhs, thread::id rhs) noexcept
    {
        return lhs.id_ < rhs.id_;
    }

    bool operator<=(thread::id lhs, thread::id rhs) noexcept
    {
        return !(rhs < lhs);
    }

    bool operator>(thread::id lhs, thread::id rhs) noexcept
    {
        return rhs < lhs;
    }

    bool operator>=(thread::id lhs, thread::id rhs) noexcept
    {
        return !(lhs < rhs);
    }
}
