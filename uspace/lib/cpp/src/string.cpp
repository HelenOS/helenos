/*
 * Copyright (c) 2019 Jaroslav Jindrak
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

#include <cassert>
#include <string>

namespace std
{
    int stoi(const string& str, size_t* idx, int base)
    {
        // TODO: implement using stol once we have numeric limits
        __unimplemented();
        return 0;
    }

    long stol(const string& str, size_t* idx, int base)
    {
        char* end;
        long result = ::strtol(str.c_str(), &end, base);

        if (end != str.c_str())
        {
            if (idx)
                *idx = static_cast<size_t>(end - str.c_str());
            return result;
        }
        // TODO: no conversion -> invalid_argument
        // TODO: ERANGE in errno -> out_of_range
        return 0;
    }

    unsigned long stoul(const string& str, size_t* idx, int base)
    {
        char* end;
        unsigned long result = ::strtoul(str.c_str(), &end, base);

        if (end != str.c_str())
        {
            if (idx)
                *idx = static_cast<size_t>(end - str.c_str());
            return result;
        }
        // TODO: no conversion -> invalid_argument
        // TODO: ERANGE in errno -> out_of_range
        return 0;
    }

    long long stoll(const string& str, size_t* idx, int base)
    {
        // TODO: implement using stol once we have numeric limits
        __unimplemented();
        return 0;
    }

    unsigned long long stoull(const string& str, size_t* idx, int base)
    {
        // TODO: implement using stoul once we have numeric limits
        __unimplemented();
        return 0;
    }

    float stof(const string& str, size_t* idx)
    {
        // TODO: implement
        __unimplemented();
        return 0.f;
    }

    double stod(const string& str, size_t* idx)
    {
        // TODO: implement
        __unimplemented();
        return 0.0;
    }

    long double stold(const string& str, size_t* idx)
    {
        // TODO: implement
        __unimplemented();
        return 0.0l;
    }

    string to_string(int val)
    {
        char* tmp;
        ::asprintf(&tmp, "%d", val);

        std::string res{tmp};
        free(tmp);

        return res;
    }

    string to_string(unsigned val)
    {
        char* tmp;
        ::asprintf(&tmp, "%u", val);

        std::string res{tmp};
        free(tmp);

        return res;
    }

    string to_string(long val)
    {
        char* tmp;
        ::asprintf(&tmp, "%ld", val);

        std::string res{tmp};
        free(tmp);

        return res;
    }

    string to_string(unsigned long val)
    {
        char* tmp;
        ::asprintf(&tmp, "%lu", val);

        std::string res{tmp};
        free(tmp);

        return res;
    }

    string to_string(long long val)
    {
        char* tmp;
        ::asprintf(&tmp, "%lld", val);

        std::string res{tmp};
        free(tmp);

        return res;
    }

    string to_string(unsigned long long val)
    {
        char* tmp;
        ::asprintf(&tmp, "%llu", val);

        std::string res{tmp};
        free(tmp);

        return res;
    }

    string to_string(float val)
    {
        char* tmp;
        ::asprintf(&tmp, "%f", val);

        std::string res{tmp};
        free(tmp);

        return res;
    }

    string to_string(double val)
    {
        char* tmp;
        ::asprintf(&tmp, "%f", val);

        std::string res{tmp};
        free(tmp);

        return res;
    }

    string to_string(long double val)
    {
        char* tmp;
        ::asprintf(&tmp, "%Lf", val);

        std::string res{tmp};
        free(tmp);

        return res;
    }

    int stoi(const wstring& str, size_t* idx, int base)
    {
        // TODO: implement
        __unimplemented();
        return 0;
    }

    long stol(const wstring& str, size_t* idx, int base)
    {
        // TODO: implement
        __unimplemented();
        return 0;
    }

    unsigned long stoul(const wstring& str, size_t* idx, int base)
    {
        // TODO: implement
        __unimplemented();
        return 0;
    }

    long long stoll(const wstring& str, size_t* idx, int base)
    {
        // TODO: implement
        __unimplemented();
        return 0;
    }

    unsigned long long stoull(const wstring& str, size_t* idx, int base)
    {
        // TODO: implement
        __unimplemented();
        return 0;
    }

    float stof(const wstring& str, size_t* idx)
    {
        // TODO: implement
        __unimplemented();
        return 0.f;
    }

    double stod(const wstring& str, size_t* idx)
    {
        // TODO: implement
        __unimplemented();
        return 0.0;
    }

    long double stold(const wstring& str, size_t* idx)
    {
        // TODO: implement
        __unimplemented();
        return 0.0l;
    }

    wstring to_wstring(int val)
    {
        // TODO: implement
        __unimplemented();
        return wstring{};
    }

    wstring to_wstring(unsigned val)
    {
        // TODO: implement
        __unimplemented();
        return wstring{};
    }

    wstring to_wstring(long val)
    {
        // TODO: implement
        __unimplemented();
        return wstring{};
    }

    wstring to_wstring(unsigned long val)
    {
        // TODO: implement
        __unimplemented();
        return wstring{};
    }

    wstring to_wstring(long long val)
    {
        // TODO: implement
        __unimplemented();
        return wstring{};
    }

    wstring to_wstring(unsigned long long val)
    {
        // TODO: implement
        __unimplemented();
        return wstring{};
    }

    wstring to_wstring(float val)
    {
        // TODO: implement
        __unimplemented();
        return wstring{};
    }

    wstring to_wstring(double val)
    {
        // TODO: implement
        __unimplemented();
        return wstring{};
    }

    wstring to_wstring(long double val)
    {
        // TODO: implement
        __unimplemented();
        return wstring{};
    }

    /**
     * 21.7, suffix for basic_string literals:
     */

    /**
     * Note: According to the standard, literal suffixes that do not
     *       start with an underscore are reserved for future standardization,
     *       but since we are implementing the standard, we're going to ignore it.
     *       This should work (according to their documentation) work for clang,
     *       but that should be tested.
     */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wliteral-suffix"
inline namespace literals {
inline namespace string_literals
{
    string operator "" s(const char* str, size_t len)
    {
        return string{str, len};
    }

    u16string operator "" s(const char16_t* str, size_t len)
    {
        return u16string{str, len};
    }

    u32string operator "" s(const char32_t* str, size_t len)
    {
        return u32string{str, len};
    }

    /* Problem: wchar_t == int in HelenOS, but standard forbids it.
    wstring operator "" s(const wchar_t* str, size_t len)
    {
        return wstring{str, len};
    }
    */
}}
#pragma GCC diagnostic pop
}
