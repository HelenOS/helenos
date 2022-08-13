/*
 * SPDX-FileCopyrightText: 2017 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_LOCALE
#define LIBCPP_BITS_LOCALE

#include <__bits/string/string.hpp>

namespace std
{
    class locale;

    /**
     * 22.3.1, class locale:
     */

    class locale
    {
        public:
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
            /* friend const Facet& use_facet(const locale&); */
            friend Facet use_facet(const locale&);

            template<class Facet>
            /* const Facet& get_() const */
            Facet get_() const
            {
                return Facet{0U};
            }
    };

    template<class Facet>
    /* const Facet& use_facet(const locale& loc) */
    Facet use_facet(const locale& loc)
    {
        return loc.get_<Facet>();
    }

    template<class Facet>
    bool has_facet(const locale& loc)
    {
        return loc.has_<Facet>();
    }
}

#endif
