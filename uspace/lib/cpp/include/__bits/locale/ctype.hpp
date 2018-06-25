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

#ifndef LIBCPP_BITS_LOCALE_CTYPE
#define LIBCPP_BITS_LOCALE_CTYPE

#include <__bits/locale/locale.hpp>
#include <__bits/string/string.hpp>
#include <cctype>

namespace std
{
    /**
     * 22.4.1, the type category:
     */

    class ctype_base
    {
        public:
            using mask = uint16_t;

            static constexpr mask space  = 0b00'0000'0001;
            static constexpr mask print  = 0b00'0000'0010;
            static constexpr mask cntrl  = 0b00'0000'0100;
            static constexpr mask upper  = 0b00'0000'1000;
            static constexpr mask lower  = 0b00'0001'0000;
            static constexpr mask alpha  = 0b00'0010'0000;
            static constexpr mask digit  = 0b00'0100'0000;
            static constexpr mask punct  = 0b00'1000'0000;
            static constexpr mask xdigit = 0b01'0000'0000;
            static constexpr mask blank  = 0b10'0000'0000;
            static constexpr mask alnum  = alpha | digit;
            static constexpr mask graph  = alnum | punct;
    };

    /**
     * 22.4.1.1, class template ctype:
     */

    template<class Char>
    class ctype: public locale::facet, public ctype_base
    {
        public:
            using char_type = Char;

            explicit ctype(size_t)
            { /* DUMMY BODY */ }

            bool is(mask m, char_type c) const
            {
                return do_is(m, c);
            }

            const char_type* is(const char_type* low, const char_type* high,
                                mask* vec) const
            {
                return do_is(low, high, vec);
            }

            const char_type* scan_is(mask m, const char_type* low,
                                     const char_type* high) const
            {
                return do_scan_is(m, low, high);
            }

            const char_type* scan_not(mask m, const char_type* low,
                                      const char_type* high) const
            {
                return do_scan_not(m, low, high);
            }

            char_type toupper(char_type c) const
            {
                return do_toupper(c);
            }

            const char_type* toupper(char_type* low, const char_type* high) const
            {
                return do_toupper(low, high);
            }

            char_type tolower(char_type c) const
            {
                return do_tolower(c);
            }

            const char_type* tolower(char_type* low, const char_type* high) const
            {
                return do_tolower(low, high);
            }

            char_type widen(char c) const
            {
                return do_widen(c);
            }

            const char_type* widen(const char* low, const char* high, char_type* to) const
            {
                return do_widen(low, high, to);
            }

            char narrow(char_type c, char def) const
            {
                return do_narrow(c, def);
            }

            const char_type* narrow(const char_type* low, const char_type* high,
                                    char def, char* to) const
            {
                return do_narrow(low, high, def, to);
            }

            static locale::id id;

            /**
             * Note: This is a deviation from the standard, because in the
             *       ISO C++ Standard this function is protected (because
             *       the instances are reference counted, but in our design
             *       they are not and as such we did this change).
             *       (This applies to all constructors of all facets.)
             */
            ~ctype() = default;

        protected:
            virtual bool do_is(mask m, char_type c) const
            {
                // TODO: implement
                return false;
            }

            virtual const char_type* do_is(const char_type* low, const char_type high,
                                           mask m) const
            {
                // TODO: implement
                return high;
            }

            virtual const char_type* do_scan_is(mask m, const char_type* low,
                                                const char_type* high) const
            {
                // TODO: implement
                return high;
            }

            virtual const char_type* do_scan_not(mask m, const char_type* low,
                                                 const char_type* high) const
            {
                // TODO: implement
                return low;
            }

            virtual char_type do_toupper(char_type c) const
            {
                // TODO: implement
                return c;
            }

            virtual const char_type* do_toupper(char_type* low, const char_type* high) const
            {
                // TODO: implement
                return high;
            }

            virtual char_type do_tolower(char_type c) const
            {
                // TODO: implement
                return c;
            }

            virtual const char_type* do_tolower(char_type* low, const char_type* high) const
            {
                // TODO: implement
                return high;
            }

            virtual char_type do_widen(char c) const
            {
                // TODO: implement
                return c;
            }

            virtual const char_type* do_widen(const char* low, const char* high,
                                              char_type* dest) const
            {
                // TODO: implement
                return high;
            }

            virtual char do_narrow(char_type c, char def) const
            {
                // TODO: implement
                return c;
            }

            virtual const char_type* do_narrow(const char_type* low, const char_type* high,
                                               char def, char* dest) const
            {
                // TODO: implement
                return high;
            }
    };

    template<class Char>
    locale::id ctype<Char>::id{};

    /**
     * 22.4.1.2, class template ctype_byname:
     * Note: Dummy, TODO: implement.
     */

    template<class Char>
    class ctype_byname: public ctype<Char>
    {
        public:
            using mask = typename ctype<Char>::mask;

            explicit ctype_byname(const char* name, size_t = 0)
            { /* DUMMY BODY */ }

            explicit ctype_byname(const string& name, size_t = 0)
            { /* DUMMY BODY */ }

        protected:
            ~ctype_byname() = default;
    };

    /**
     * 22.4.1.3, ctype specialziations:
     */

    template<>
    class ctype<char>: public locale::facet, public ctype_base
    {
        public:
            using char_type = char;

            explicit ctype(const mask* tab = nullptr, bool del = false, size_t = 0)
            { /* DUMMY BODY */ }

            bool is(mask m, char_type c) const
            {
                return do_is(m, c);
            }

            const char_type* is(const char_type* low, const char_type* high,
                                mask* vec) const
            {
                return do_is(low, high, vec);
            }

            const char_type* scan_is(mask m, const char_type* low,
                                     const char_type* high) const
            {
                return do_scan_is(m, low, high);
            }

            const char_type* scan_not(mask m, const char_type* low,
                                      const char_type* high) const
            {
                return do_scan_not(m, low, high);
            }

            char_type toupper(char_type c) const
            {
                return do_toupper(c);
            }

            const char_type* toupper(char_type* low, const char_type* high) const
            {
                return do_toupper(low, high);
            }

            char_type tolower(char_type c) const
            {
                return do_tolower(c);
            }

            const char_type* tolower(char_type* low, const char_type* high) const
            {
                return do_tolower(low, high);
            }

            char_type widen(char c) const
            {
                return do_widen(c);
            }

            const char_type* widen(const char* low, const char* high, char_type* to) const
            {
                return do_widen(low, high, to);
            }

            char narrow(char_type c, char def) const
            {
                return do_narrow(c, def);
            }

            const char_type* narrow(const char_type* low, const char_type* high,
                                    char def, char* to) const
            {
                return do_narrow(low, high, def, to);
            }

            static locale::id id;
            static const size_t table_size{0};

            const mask* table() const noexcept
            {
                return classic_table();
            }

            static const mask* classic_table() noexcept
            {
                return classic_table_;
            }

            ~ctype() = default;

        protected:
            virtual bool do_is(mask m, char_type c) const
            {
                // TODO: implement, this is a dummy
                if ((m & space) != 0 && std::isspace(c))
                    return true;
                else if ((m & alpha) != 0 && std::isalpha(c))
                    return true;
                else if ((m & upper) != 0 && std::isupper(c))
                    return true;
                else if ((m & lower) != 0 && std::islower(c))
                    return true;
                else if ((m & digit) != 0 && std::isdigit(c))
                    return true;
                return false;
            }

            virtual const char_type* do_is(const char_type* low, const char_type* high,
                                           mask* m) const
            {
                // TODO: implement
                return high;
            }

            virtual const char_type* do_scan_is(mask m, const char_type* low,
                                                const char_type* high) const
            {
                // TODO: implement
                return high;
            }

            virtual const char_type* do_scan_not(mask m, const char_type* low,
                                                 const char_type* high) const
            {
                // TODO: implement
                return high;
            }

            virtual char_type do_toupper(char_type c) const
            {
                return std::toupper(c);
            }

            virtual const char_type* do_toupper(char_type* low, const char_type* high) const
            {
                while (low != high)
                    *low = std::toupper(*low);

                return high;
            }

            virtual char_type do_tolower(char_type c) const
            {
                return std::tolower(c);
            }

            virtual const char_type* do_tolower(char_type* low, const char_type* high) const
            {
                while (low != high)
                    *low = std::tolower(*low);

                return high;
            }

            virtual char_type do_widen(char c) const
            {
                return c;
            }

            virtual const char_type* do_widen(const char* low, const char* high,
                                              char_type* dest) const
            {
                while (low != high)
                    *dest++ = *low++;

                return high;
            }

            virtual char do_narrow(char_type c, char def) const
            {
                return c;
            }

            virtual const char_type* do_narrow(const char_type* low, const char_type* high,
                                               char def, char* dest) const
            {
                while (low != high)
                    *dest++ = *low++;

                return high;
            }

        private:
            static constexpr mask* classic_table_{nullptr};
    };

    template<>
    class ctype<wchar_t>: public locale::facet, public ctype_base
    {
        public:
            using char_type = wchar_t;

            explicit ctype(const mask* tab = nullptr, bool del = false, size_t = 0)
            { /* DUMMY BODY */ }

            bool is(mask m, char_type c) const
            {
                return do_is(m, c);
            }

            const char_type* is(const char_type* low, const char_type* high,
                                mask* vec) const
            {
                return do_is(low, high, vec);
            }

            const char_type* scan_is(mask m, const char_type* low,
                                     const char_type* high) const
            {
                return do_scan_is(m, low, high);
            }

            const char_type* scan_not(mask m, const char_type* low,
                                      const char_type* high) const
            {
                return do_scan_not(m, low, high);
            }

            char_type toupper(char_type c) const
            {
                return do_toupper(c);
            }

            const char_type* toupper(char_type* low, const char_type* high) const
            {
                return do_toupper(low, high);
            }

            char_type tolower(char_type c) const
            {
                return do_tolower(c);
            }

            const char_type* tolower(char_type* low, const char_type* high) const
            {
                return do_tolower(low, high);
            }

            char_type widen(char c) const
            {
                return do_widen(c);
            }

            const char_type* widen(const char* low, const char* high, char_type* to) const
            {
                return do_widen(low, high, to);
            }

            char narrow(char_type c, char def) const
            {
                return do_narrow(c, def);
            }

            const char_type* narrow(const char_type* low, const char_type* high,
                                    char def, char* to) const
            {
                return do_narrow(low, high, def, to);
            }

            static locale::id id;
            static const size_t table_size{0};

            const mask* table() const noexcept
            {
                return classic_table();
            }

            static const mask* classic_table() noexcept
            {
                return classic_table_;
            }

            ~ctype() = default;

        protected:
            virtual bool do_is(mask m, char_type c) const
            {
                // TODO: implement
                return false;
            }

            virtual const char_type* do_is(const char_type* low, const char_type* high,
                                           mask* m) const
            {
                // TODO: implement
                return high;
            }

            virtual const char_type* do_scan_is(mask m, const char_type* low,
                                                const char_type* high) const
            {
                // TODO: implement
                return high;
            }

            virtual const char_type* do_scan_not(mask m, const char_type* low,
                                                 const char_type* high) const
            {
                // TODO: implement
                return high;
            }

            virtual char_type do_toupper(char_type c) const
            {
                // TODO: implement
                return c;
            }

            virtual const char_type* do_toupper(char_type* low, const char_type* high) const
            {
                // TODO: implement
                return high;
            }

            virtual char_type do_tolower(char_type c) const
            {
                // TODO: implement
                return c;
            }

            virtual const char_type* do_tolower(char_type* low, const char_type* high) const
            {
                // TODO: implement
                return high;
            }

            virtual char_type do_widen(char c) const
            {
                // TODO: implement
                return c;
            }

            virtual const char_type* do_widen(const char* low, const char* high,
                                              char_type* dest) const
            {
                // TODO: implement
                return dest;
            }

            virtual char do_narrow(char_type c, char def) const
            {
                // TODO: implement
                return c;
            }

            virtual const char_type* do_narrow(const char_type* low, const char_type* high,
                                               char def, char* dest) const
            {
                // TODO: implement
                return high;
            }

        private:
            static constexpr mask* classic_table_{nullptr};
    };
}

#endif
