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
