/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <cstdlib>
#include <new>

namespace std
{
    const char* bad_alloc::what() const noexcept
    {
        return "std::bad_alloc";
    }

    const nothrow_t nothrow{};

    static new_handler handler = nullptr;

    new_handler set_new_handler(new_handler h)
    {
        auto old = handler;
        handler = h;

        return old;
    }

    new_handler get_new_handler() noexcept
    {
        return handler;
    }
}

void* operator new(std::size_t size)
{
    if (size == 0)
        size = 1;

    void *ptr = std::malloc(size);

    while (!ptr)
    {
        auto h = std::get_new_handler();
        if (h)
            h();
        else
        {
            // TODO: For this we need stack unwinding support.
            /*     throw std::bad_alloc{}; */
        }
    }

    return ptr;
}

void* operator new(std::size_t ignored, void* ptr)
{ // Placement new.
    return ptr;
}

void* operator new(std::size_t size, const std::nothrow_t& nt) noexcept
{
    void* ptr{nullptr};

    try
    {
        ptr = ::operator new(size);
    }
    catch(...)
    { /* DUMMY BODY */ }

    return ptr;
}

void* operator new[](std::size_t size)
{
    return ::operator new(size);
}

void* operator new[](std::size_t size, const std::nothrow_t& nt) noexcept
{
    return ::operator new(size, nt);
}

void operator delete(void* ptr) noexcept
{
    if (ptr)
        std::free(ptr);
}

void operator delete(void* ptr, std::size_t ignored) noexcept
{
    ::operator delete(ptr);
}

void operator delete[](void* ptr) noexcept
{
    ::operator delete(ptr);
}

void operator delete[](void* ptr, std::size_t ignored) noexcept
{
    ::operator delete(ptr);
}
