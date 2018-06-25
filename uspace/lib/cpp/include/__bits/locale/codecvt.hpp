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

#ifndef LIBCPP_BITS_LOCALE_CODECVT
#define LIBCPP_BITS_LOCALE_CODECVT

#include <__bits/locale/locale.hpp>
#include <string>

namespace std
{
    /**
     * 22.4.1.4, class template codecvt:
     */

    class codecvt_base
    {
        public:
            enum result
            {
                ok, partial, error, noconv
            };
    };

    template<class Intern, class Extern, class State>
    class codecvt: public codecvt_base
    {
        public:
            using intern_type = Intern;
            using extern_type = Extern;
            using state_type  = State;

            explicit codecvt(size_t = 0)
            { /* DUMMY BODY */ }

            result out(state_type& state, const intern_type* from, const intern_type* from_end,
                       const intern_type*& from_next, extern_type* to, extern_type* to_end,
                       extern_type*& to_next) const
            {
                return do_out(state, from, from_end, from_next, to, to_end, to_next);
            }

            result unshift(state_type& state, extern_type* to, extern_type* to_end,
                           extern_type*& to_next) const
            {
                return do_unshift(state, to, to_end, to_next);
            }

            result in(state_type& state, const extern_type* from, const extern_type* from_end,
                      const extern_type*& from_next, intern_type* to, intern_type* to_end,
                      intern_type*& to_next) const
            {
                return do_in(state, from, from_end, from_next, to, to_end, to_next);
            }

            int encoding() const noexcept
            {
                return do_encoding();
            }

            bool always_noconv() const noexcept
            {
                return do_always_noconv();
            }

            int length(state_type& state, const extern_type* from, const extern_type* end,
                       size_t max) const
            {
                return do_length(state, from, end, max);
            }

            int max_length() const noexcept
            {
                return do_max_length();
            }

            static locale::id id;

            ~codecvt() = default;

        protected:
            virtual result do_out(state_type& state, const intern_type* from, const intern_type* from_end,
                                  const intern_type*& from_next, extern_type* to, extern_type* to_end,
                                  extern_type*& to_next) const
            {
                // TODO: implement
                return error;
            }

            virtual result do_unshift(state_type& state, extern_type* to, extern_type* to_end,
                                      extern_type*& to_next) const
            {
                // TODO: implement
                return error;
            }

            virtual result do_in(state_type& state, const extern_type* from, const extern_type* from_end,
                                 const extern_type*& from_next, intern_type* to, intern_type* to_end,
                                 intern_type*& to_next) const
            {
                // TODO: implement
                return error;
            }

            virtual int do_encoding() const noexcept
            {
                // TODO: implement
                return 0;
            }

            virtual bool do_always_noconv() const noexcept
            {
                // TODO: implement
                return false;
            }

            virtual int do_length(state_type& state, const extern_type* from, const extern_type* end,
                                  size_t max) const
            {
                // TODO: implement
                return 0;
            }

            virtual int do_max_length() const noexcept
            {
                // TODO: implement
                return 0;
            }
    };

    template<class Intern, class Extern, class State>
    locale::id codecvt<Intern, Extern, State>::id{};

    /**
     * 22.4.1.5, class template codecvt_byname:
     * Note: Dummy, TODO: implement.
     */

    template<class Intern, class Extern, class State>
    class codecvt_byname: public codecvt<Intern, Extern, State>
    {
        public:
            explicit codecvt_byname(const char*, size_t = 0)
            { /* DUMMY BODY */ }

            explicit codecvt_byname(const string&, size_t = 0)
            { /* DUMMY BODY */ }

            ~codecvt_byname() = default;
    };
}

#endif
