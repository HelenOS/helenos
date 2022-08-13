/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_TYPE_INFO
#define LIBCPP_BITS_TYPE_INFO

#include <cstdlib>

namespace std
{
    class type_info
    {
        public:
            virtual ~type_info();

            bool operator==(const type_info&) const noexcept;
            bool operator!=(const type_info&) const noexcept;

            bool before(const type_info&) const noexcept;

            size_t hash_code() const noexcept;

            const char* name() const noexcept;

            type_info(const type_info&) = delete;
            type_info& operator=(const type_info&) = delete;

        private:
            const char* __name;
    };

    // TODO: class bad_cast, bad_typeid
}

#endif

