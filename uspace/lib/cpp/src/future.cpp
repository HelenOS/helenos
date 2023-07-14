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

#include <future>
#include <string>
#include <system_error>

namespace std
{
    error_code make_error_code(future_errc ec) noexcept
    {
        return error_code{static_cast<int>(ec), future_category()};
    }

    error_condition make_error_condition(future_errc ec) noexcept
    {
        return error_condition{static_cast<int>(ec), future_category()};
    }

    namespace aux
    {
        class future_category_t: public error_category
        {
            public:
                future_category_t()
                    : error_category{}
                { /* DUMMY BODY */ }

                ~future_category_t() = default;

                const char* name() const noexcept override
                {
                    return "future";
                }

                string message(int ev) const override
                {
                    return "ev: " + std::to_string(ev);
                }
        };
    }

    const error_category& future_category() noexcept
    {
        static aux::future_category_t instance{};

        return instance;
    }

    future_error::future_error(error_code ec)
        : logic_error{"future_error"}, code_{ec}
    { /* DUMMY BODY */ }

    const error_code& future_error::code() const noexcept
    {
        return code_;
    }

    const char* future_error::what() const noexcept
    {
        /*
         * FIXME
         * Following code would be optimal but the message string is
         * actually destroyed before the function returns so we would
         * be returning a dangling pointer. No simple fix available, hence
         * we use the ugly hack.
         */
        //return code().message().c_str();
#define QUOTE_ARG(arg) #arg
        return "future_error, see " __FILE__ ":" QUOTE_ARG(__LINE__);
    }
}
