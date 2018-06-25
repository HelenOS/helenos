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

#ifndef LIBCPP_BITS_ITERATOR_HELPERS
#define LIBCPP_BITS_ITERATOR_HELPERS

#include <__bits/aux.hpp>

namespace std::aux
{
    /**
     * Used for our custom iterators where we know
     * that their references/const_references and
     * pointers/const_pointers differ only in constness.
     */

    template<class T>
    struct get_non_const_ref
        : type_is<T>
    { /* DUMMY BODY */ };

    template<class T>
    struct get_non_const_ref<const T&>
        : type_is<T&>
    { /* DUMMY BODY */ };

    template<class T>
    using get_non_const_ref_t = typename get_non_const_ref<T>::type;

    template<class T>
    struct get_non_const_ptr
        : type_is<T>
    { /* DUMMY BODY */ };

    template<class T>
    struct get_non_const_ptr<const T*>
        : type_is<T*>
    { /* DUMMY BODY */ };

    template<class T>
    using get_non_const_ptr_t = typename get_non_const_ptr<T>::type;
}

#endif
