/*
 * SPDX-FileCopyrightText: 2019 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_NEW
#define LIBCPP_BITS_NEW

#include <cstddef>
#include <exception>

namespace std
{

class bad_alloc: public std::exception
{
	public:
		bad_alloc() = default;
		bad_alloc(const bad_alloc&) = default;
		bad_alloc& operator=(const bad_alloc&) = default;
		virtual const char* what() const noexcept override;
		virtual ~bad_alloc() = default;
};

struct nothrow_t {};
extern const nothrow_t nothrow;

using new_handler = void (*)();

new_handler set_new_handler(new_handler);
new_handler get_new_handler() noexcept;

}

void* operator new(std::size_t);
void* operator new(std::size_t, void*);
void* operator new(std::size_t, const std::nothrow_t&) noexcept;
void* operator new[](std::size_t);
void* operator new[](std::size_t, const std::nothrow_t&) noexcept;

void operator delete(void*) noexcept;
void operator delete(void*, std::size_t) noexcept;
void operator delete[](void*) noexcept;
void operator delete[](void*, std::size_t) noexcept;

#endif

