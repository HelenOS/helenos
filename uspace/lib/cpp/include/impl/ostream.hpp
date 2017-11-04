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

#ifndef LIBCPP_OSTREAM
#define LIBCPP_OSTREAM

#include <iosfwd>

namespace std
{
    template<class Char, class Traits = char_traits<Char>>
    class basic_ostream;

    using ostream  = basic_ostream<char>;
    using wostream = basic_ostream<wchar_t>;

    template<class Char, class Traits = char_traits<Char>>
    basic_ostream<Char, Traits>& endl(basic_ostream<Char, Traits>& os);

    template<class Char, class Traits = char_traits<Char>>
    basic_ostream<Char, Traits>& ends(basic_ostream<Char, Traits>& os);

    template<class Char, class Traits = char_traits<Char>>
    basic_ostream<Char, Traits>& flush(basic_ostream<Char, Traits>& os);

    template<class Char, class Tratis = char_traits<Char>>
    basic_ostream<Char, Traits>& operator<<(basic_ostream<Char, Traits>& os, const T& x);
}

#endif

