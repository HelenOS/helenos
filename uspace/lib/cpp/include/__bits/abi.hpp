/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_ABI
#define LIBCPP_BITS_ABI

#include <cstddef>
#include <cstdint>
#include <typeinfo>

namespace __cxxabiv1
{
    /**
     * Static constructor/destructor helpers.
     */

    extern "C" int __cxa_atexit(void (*)(void*), void*, void*);

    extern "C" void __cxa_finalize(void*);

    /**
     * Itanium C++ ABI type infos.
     * See section 2.9.4 (RTTI Layout) of the Itanium C++ ABI spec.
     *
     * Note: The memory representation of these classes must not
     *       be modified! (Wel, no modifications at all shall be done.)
     *
     * Source: https://itanium-cxx-abi.github.io/cxx-abi/abi.html
     */

    class __fundamental_type_info: public std::type_info
    {
        public:
            virtual ~__fundamental_type_info();
    };

    class __array_type_info: public std::type_info
    {
        public:
            virtual ~__array_type_info();
    };

    class __function_type_info: public std::type_info
    {
        public:
            virtual ~__function_type_info();
    };

    class __enum_type_info: public std::type_info
    {
        public:
            virtual ~__enum_type_info();
    };

    class __class_type_info: public std::type_info
    {
        public:
            virtual ~__class_type_info();
    };

    class __si_class_type_info: public __class_type_info
    {
        public:
            virtual ~__si_class_type_info();

            const __class_type_info* __base_type;
    };

    struct __base_class_type_info
    {
        const __class_type_info* __base_type;
        long __offset_flags;

        enum __ofset_flags_masks
        {
            __virtual_mask = 0x1,
            __public_mask =  0x2,
            __offset_shift = 0x8
        };
    };

    class __vmi_class_type_info: public __class_type_info
    {
        public:
            virtual ~__vmi_class_type_info();

            std::uint32_t __flags;
            std::uint32_t __base_count;

            __base_class_type_info __base_info[1];

            enum __flags_mask
            {
                __non_diamond_repeat_mask = 0x1,
                __diamond_shaped_mask     = 0x2
            };
    };

    class __pbase_type_info: public std::type_info
    {
        public:
            virtual ~__pbase_type_info();

            std::uint32_t __flags;
            const std::type_info* __pointee;

            enum __masks
            {
                __const_mask            = 0x01,
                __volatile_mask         = 0x02,
                __restrict_mask         = 0x04,
                __incomplete_mask       = 0x08,
                __incomplete_class_mask = 0x10,
                __transaction_safe_mask = 0x20,
                __noexcept_mask         = 0x40
            };
    };

    class __pointer_type_info: public __pbase_type_info
    {
        public:
            virtual ~__pointer_type_info();
    };

    class __pointer_to_member_type_info: public __pbase_type_info
    {
        public:
            virtual ~__pointer_to_member_type_info();

            const __class_type_info* __context;
    };

    extern "C" void* __dynamic_cast(const void*, const __class_type_info*,
                                    const __class_type_info*, std::ptrdiff_t);
}

namespace abi = __cxxabiv1;

#endif
