/*
 * SPDX-FileCopyrightText: 2019 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cassert>
#include <cstdlib>
#include <exception>
#include <thread>
#include <utility>

namespace std
{
    thread::thread() noexcept
        : id_{}
    { /* DUMMY BODY */ }

    thread::~thread()
    {
        // TODO: investigate joinable() in detail
        //       + std::terminate behaves weirdly on HelenOS
        if (joinable() && false)
            std::terminate();

        // TODO: check for finished too?
        // TODO: WAIT! if it's not detached, then
        //       we are joinable and std::terminate was called?
        // TODO: review this entire thing
        if (joinable_wrapper_ && !joinable_wrapper_->detached())
            delete joinable_wrapper_;
    }

    thread::thread(thread&& other) noexcept
        : id_{other.id_}, joinable_wrapper_{other.joinable_wrapper_}
    {
        other.id_ = aux::thread_t{};
        other.joinable_wrapper_ = nullptr;
    }

    thread& thread::operator=(thread&& other) noexcept
    {
        if (joinable())
            std::terminate();

        id_ = other.id_;
        other.id_ = aux::thread_t{};

        joinable_wrapper_ = other.joinable_wrapper_;
        other.joinable_wrapper_ = nullptr;

        return *this;
    }

    void thread::swap(thread& other) noexcept
    {
        std::swap(id_, other.id_);
        std::swap(joinable_wrapper_, other.joinable_wrapper_);
    }

    bool thread::joinable() const noexcept
    {
        return id_ != aux::thread_t{};
    }

    void thread::join()
    {
        if (joinable() && joinable_wrapper_)
            joinable_wrapper_->join();
    }

    void thread::detach()
    {
        id_ = aux::thread_t{};

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
        __unimplemented();
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
            return thread::id{aux::threading::thread::this_thread()};
        }

        void yield() noexcept
        {
            aux::threading::thread::yield();
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
