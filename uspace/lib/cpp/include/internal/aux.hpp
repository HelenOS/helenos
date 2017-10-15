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

#ifndef LIBCPP_AUX
#define LIBCPP_AUX

namespace std
{

namespace aux
{
    /**
     * Two very handy templates, this allows us
     * to easily follow the T::type and T::value
     * convention by simply inheriting from specific
     * instantiations of these templates.
     * Examples:
     *  1) We need a struct with int typedef'd to type:
     *
     *      stuct has_type_int: aux::type<int> {};
     *      typename has_type_int::type x = 1; // x is of type int
     *
     *  2) We need a struct with static size_t member called value:
     *
     *      struct has_value_size_t: aux::value<size_t, 1u> {};
     *      std::printf("%u\n", has_value_size_t::value); // prints "1\n"
     */

    template<class T>
    struct type_is
    {
        using type = T;
    };

    template<class T, T v>
    struct value_is
    {
        static constexpr T value = v;
    };
}

}

#endif
