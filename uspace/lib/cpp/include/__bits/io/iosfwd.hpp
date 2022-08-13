/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCPP_BITS_IO_IOSFWD
#define LIBCPP_BITS_IO_IOSFWD

namespace std
{
    /**
     * 27.3, forward declarations:
     */

    template<class Char>
    class char_traits;

    template<>
    class char_traits<char>;

    template<>
    class char_traits<char16_t>;

    template<>
    class char_traits<char32_t>;

    template<>
    class char_traits<wchar_t>;



    template<class T>
    class allocator;



    template<class Char, class Traits = char_traits<Char>>
    class basic_ios;

    template<class Char, class Traits = char_traits<Char>>
    class basic_streambuf;

    template<class Char, class Traits = char_traits<Char>>
    class basic_istream;

    template<class Char, class Traits = char_traits<Char>>
    class basic_ostream;

    template<class Char, class Traits = char_traits<Char>>
    class basic_iostream;



    template<class Char, class Traits = char_traits<Char>,
             class Allocator = allocator<Char>>
    class basic_stringbuf;

    template<class Char, class Traits = char_traits<Char>,
             class Allocator = allocator<Char>>
    class basic_istringstream;

    template<class Char, class Traits = char_traits<Char>,
             class Allocator = allocator<Char>>
    class basic_ostringstream;

    template<class Char, class Traits = char_traits<Char>,
             class Allocator = allocator<Char>>
    class basic_stringstream;



    template<class Char, class Traits = char_traits<Char>>
    class basic_filebuf;

    template<class Char, class Traits = char_traits<Char>>
    class basic_ifstream;

    template<class Char, class Traits = char_traits<Char>>
    class basic_ofstream;

    template<class Char, class Traits = char_traits<Char>>
    class basic_fstream;



    template<class Char, class Traits = char_traits<Char>>
    class istreambuf_iterator;

    template<class Char, class Traits = char_traits<Char>>
    class ostreambuf_iterator;

    using ios  = basic_ios<char>;
    using wios = basic_ios<wchar_t>;

    using streambuf = basic_streambuf<char>;
    using istream   = basic_istream<char>;
    using ostream   = basic_ostream<char>;
    using iostream  = basic_iostream<char>;

    using stringbuf     = basic_stringbuf<char>;
    using istringstream = basic_istringstream<char>;
    using ostringstream = basic_ostringstream<char>;
    using stringstream  = basic_stringstream<char>;

    using filebuf  = basic_filebuf<char>;
    using ifstream = basic_ifstream<char>;
    using ofstream = basic_ofstream<char>;
    using fstream  = basic_fstream<char>;

    using wstreambuf = basic_streambuf<wchar_t>;
    using wistream   = basic_istream<wchar_t>;
    using wostream   = basic_ostream<wchar_t>;
    using wiostream  = basic_iostream<wchar_t>;

    using wstringbuf     = basic_stringbuf<wchar_t>;
    using wistringstream = basic_istringstream<wchar_t>;
    using wostringstream = basic_ostringstream<wchar_t>;
    using wstringstream  = basic_stringstream<wchar_t>;

    using wfilebuf  = basic_filebuf<wchar_t>;
    using wifstream = basic_ifstream<wchar_t>;
    using wofstream = basic_ofstream<wchar_t>;
    using wfstream  = basic_fstream<wchar_t>;

    template<class State>
    class fpos;

    // TODO: standard p. 1012, this is circular, it offers fix
    /* using streampos  = fpos<char_traits<char>::state_type>; */
    /* using wstreampos = fpos<char_traits<wchar_t>::state_type>; */
    // Temporary workaround.
    using streampos =  unsigned long long;
    using wstreampos = unsigned long long;
    // TODO: This should be in ios and not here?
    using streamoff = long long;
}

#endif
