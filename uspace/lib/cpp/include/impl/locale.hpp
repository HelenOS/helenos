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

#ifndef LIBCPP_LOCALE
#define LIBCPP_LOCALE

#include <cctype>
#include <cstdlib>
#include <iosfwd>
#include <string>

namespace std
{

    /**
     * Note: This is a very simplistic implementation of <locale>.
     *       We have a single locale that has all of its facets.
     *       This should behave correctly on the outside but will prevent
     *       us from using multiple locales so if that becomes necessary
     *       in the future, TODO: implement :)
     */

    namespace aux
    {
        class facet
        {
            protected:
                explicit facet(size_t refs = 0);

                virtual ~facet();

                facet(const facet&) = delete;
                void operator=(const facet&) = delete;
        };

        class id
        {
            public:
                id() = default;

                id(const id&) = delete;
                void operator=(const id&) = delete;
        };
    }

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
    class ctype: public aux::facet, public ctype_base
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

            static aux::id id;

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
    aux::id ctype<Char>::id{};

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
    class ctype<char>: public aux::facet, public ctype_base
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

            static aux::id id;
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
    class ctype<wchar_t>: public aux::facet, public ctype_base
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

            static aux::id id;
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

            static aux::id id;

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
    aux::id codecvt<Intern, Extern, State>::id{};

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

    /**
     * 22.3.1, class locale:
     */

    class locale
    {
        public:
            using facet = aux::facet;
            using id    = aux::id;

            using category = int;

            static const category none     = 0b000'0001;
            static const category collate  = 0b000'0010;
            static const category ctype    = 0b000'0100;
            static const category monetary = 0b000'1000;
            static const category numeric  = 0b001'0000;
            static const category time     = 0b010'0000;
            static const category messages = 0b100'0000;
            static const category all      = collate | ctype | monetary |
                                             numeric | time | messages;

            locale() noexcept;

            locale(const locale& other) noexcept;

            explicit locale(const char* name);

            explicit locale(const string& name);

            locale(const locale& other, const char* name, category);

            locale(const locale& other, const string& name, category);

            template<class Facet>
            locale(const locale& other, Facet* f)
                : name_{other.name_}
            { /* DUMMY BODY */ }

            locale(const locale& other, const locale& one, category);

            ~locale() = default;

            const locale& operator=(const locale& other) noexcept;

            template<class Facet>
            locale combine(const locale& other) const
            {
                return other;
            }

            string name() const;

            bool operator==(const locale& other) const;
            bool operator!=(const locale& other) const;

            template<class Char, class Traits, class Allocator>
            bool operator()(const basic_string<Char, Traits, Allocator>& s1,
                            const basic_string<Char, Traits, Allocator>& s2) const
            {
                // TODO: define outside locale
                /* return use_facet<collate<Char>>(*this).compare( */
                /*     s1.begin(), s1.end(), s2.begin(), s2.end() */
                /* ) < 0; */
                return false;
            }

            static locale global(const locale&)
            {
                return *the_locale_;
            }

            static const locale& classic()
            {
                return *the_locale_;
            }

        private:
            string name_;

            // TODO: implement the_locale_
            static constexpr locale* the_locale_{nullptr};

            template<class Facet>
            friend bool has_facet(const locale&);

            template<class Facet>
            bool has_()
            { // Our single locale atm has all facets.
                return true;
            }

            template<class Facet>
            friend const Facet& use_facet(const locale&);

            /**
             * Note: We store all of the facets in the main
             *       locale.
             */

            template<class Facet>
            const Facet& get_();

            std::ctype<char> ctype_char_{};
            std::ctype<wchar_t> ctype_wchar_{};
    };

    template<>
    const ctype<char>& locale::get_<ctype<char>>()
    {
        return ctype_char_;
    }

    template<>
    const ctype<wchar_t>& locale::get_<ctype<wchar_t>>()
    {
        return ctype_wchar_;
    }

    template<class Facet>
    const Facet& use_facet(const locale& loc)
    {
        return loc.get_<Facet>();
    }

    template<class Facet>
    bool has_facet(const locale& loc)
    {
        return loc.has_<Facet>();
    }

    /**
     * 22.3.3, convenience interfaces:
     */

    /**
     * 22.3.3.1, character classification:
     */

    template<class Char>
    bool isspace(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).is(ctype_base::space, c);
    }

    template<class Char>
    bool isprint(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).is(ctype_base::print, c);
    }

    template<class Char>
    bool iscntrl(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).is(ctype_base::cntrl, c);
    }

    template<class Char>
    bool isupper(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).is(ctype_base::upper, c);
    }

    template<class Char>
    bool islower(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).is(ctype_base::lower, c);
    }

    template<class Char>
    bool isalpha(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).is(ctype_base::alpha, c);
    }

    template<class Char>
    bool isdigit(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).is(ctype_base::digit, c);
    }

    template<class Char>
    bool ispunct(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).is(ctype_base::punct, c);
    }

    template<class Char>
    bool isxdigit(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).is(ctype_base::xdigit, c);
    }

    template<class Char>
    bool isalnum(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).is(ctype_base::alnum, c);
    }

    template<class Char>
    bool isgraph(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).is(ctype_base::graph, c);
    }

    template<class Char>
    bool isblank(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).is(ctype_base::blank, c);
    }

    /**
     * 22.3.3.2, conversions:
     */

    /**
     * 22.3.3.2.1, character conversions:
     */

    template<class Char>
    Char toupper(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).toupper(c);
    }

    template<class Char>
    Char tolower(Char c, const locale& loc)
    {
        return use_facet<ctype<Char>>(loc).tolower(c);
    }

    /**
     * 22.3.3.2.2, string conversions:
     */

    // TODO: implement
}

#endif
