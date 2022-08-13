/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
        return code().message().c_str();
    }
}
