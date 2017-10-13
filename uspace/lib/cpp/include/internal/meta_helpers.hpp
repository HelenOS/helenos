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

namespace std
{

namespace aux
{
    template<class T>
    struct is_void: false_type;

    template<>
    struct is_void<void>: true_type;

    template<class T>
    struct is_integral: false_type;

    template<>
    struct is_integral<bool>: true_type;

    template<>
    struct is_integral<char>: true_type;

    template<>
    struct is_integral<signed char>: true_type;

    template<>
    struct is_integral<unsigned char>: true_type;

    template<>
    struct is_integral<wchar_t>: true_type;

    template<>
    struct is_integral<long>: true_type;

    template<>
    struct is_integral<unsigned long>: true_type;

    template<>
    struct is_integral<int>: true_type;

    template<>
    struct is_integral<unsigned int>: true_type;

    template<>
    struct is_integral<short>: true_type;

    template<>
    struct is_integral<unsigned short>: true_type;

    template<>
    struct is_integral<long long>: true_type;

    template<>
    struct is_integral<unsigned long long>: true_type;

    template<class T>
    struct is_floating_point: false_type;

    template<>
    struct is_floating_point<float>: true_type;

    template<>
    struct is_floating_point<double>: true_type;

    template<>
    struct is_floating_point<long double>: true_type;

    template<class>
    struct is_pointer: false_type;

    template<class T>
    struct is_pointer<T*>: true_type;
}

}
