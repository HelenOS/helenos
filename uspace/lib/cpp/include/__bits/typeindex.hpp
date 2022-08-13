/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_TYPE_INDEX
#define LIBCPP_BITS_TYPE_INDEX

#include <cstdlib>
#include <typeinfo>

namespace std
{

    /**
     * 20.14.2, type_index:
     */

    class type_index
    {
        public:
            type_index(const type_info& rhs) noexcept;

            bool operator==(const type_index& rhs) const noexcept;

            bool operator!=(const type_index& rhs) const noexcept;

            bool operator<(const type_index& rhs) const noexcept;

            bool operator<=(const type_index& rhs) const noexcept;

            bool operator>(const type_index& rhs) const noexcept;

            bool operator>=(const type_index& rhs) const noexcept;

            size_t hash_code() const noexcept;

            const char* name() const noexcept;

        private:
            const type_info* target_;
    };

    template<class T>
    struct hash;

    // TODO: implement
    template<>
    struct hash<type_index>;
}

#endif
