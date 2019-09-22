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

#ifndef LIBCPP_BITS_SYSTEM_ERROR
#define LIBCPP_BITS_SYSTEM_ERROR

#include <__bits/aux.hpp>
#include <__bits/string/stringfwd.hpp>
#include <stdexcept>
#include <type_traits>

namespace std
{
    class error_condition;
    class error_code;

    enum class errc
    { // TODO: add matching values
        address_family_not_supported,
        address_in_use,
        address_not_available,
        already_connected,
        argument_list_too_long,
        argument_out_of_domain,
        bad_address,
        bad_file_descriptor,
        bad_message,
        broken_pipe,
        connection_aborted,
        connection_already_in_progress,
        connection_refused,
        connection_reset,
        cross_device_link,
        destination_address_required,
        device_or_resource_busy,
        directory_not_empty,
        executable_format_error,
        file_exists,
        file_too_large,
        filename_too_long,
        function_not_supported,
        host_unreachable,
        identifier_removed,
        illegal_byte_sequence,
        inappropriate_io_control_operation,
        interrupted,
        invalid_argument,
        invalid_seek,
        io_error,
        is_a_directory,
        message_size,
        network_down,
        network_reset,
        network_unreachable,
        no_buffer_space,
        no_child_process,
        no_link,
        no_lock_available,
        no_message_available,
        no_message,
        no_protocol_option,
        no_space_on_device,
        no_stream_resources,
        no_such_device_or_address,
        no_such_device,
        no_such_file_or_directory,
        no_such_process,
        not_a_directory,
        not_a_socket,
        not_a_stream,
        not_connected,
        not_enough_memory,
        not_supported,
        operation_canceled,
        operation_in_progress,
        operation_not_permitted,
        operation_not_supported,
        operation_would_block,
        owner_dead,
        permission_denied,
        protocol_error,
        protocol_not_supported,
        read_only_file_system,
        resource_deadlock_would_occur,
        resource_unavailable_try_again,
        result_out_of_range,
        state_not_recoverable,
        stream_timeout,
        text_file_busy,
        timed_out,
        too_many_files_open_in_system,
        too_many_files_open,
        too_many_links,
        too_many_symbolic_link_levels,
        value_too_large,
        wrong_protocol_type
    };

    template<class>
    struct is_error_code_enum: false_type
    { /* DUMMY BODY */ };

    template<>
    struct is_error_code_enum<errc>: true_type
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_error_code_enum_v = is_error_code_enum<T>::value;

    template<class>
    struct is_error_condition_enum: false_type
    { /* DUMMY BODY */ };

    template<>
    struct is_error_condition_enum<errc>: true_type
    { /* DUMMY BODY */ };

    template<class T>
    inline constexpr bool is_error_condition_enum_v = is_error_condition_enum<T>::value;

    /**
     * 19.5.1, class error_category:
     */

    class error_category
    {
        public:
            constexpr error_category() noexcept = default;
            virtual ~error_category();

            error_category(const error_category&) = delete;
            error_category& operator=(const error_category&) = delete;

            virtual const char* name() const noexcept = 0;
            virtual error_condition default_error_condition(int) const noexcept;
            virtual bool equivalent(int, const error_condition&) const noexcept;
            virtual bool equivalent(const error_code&, int) const noexcept;
            virtual string message(int) const = 0;

            bool operator==(const error_category&) const noexcept;
            bool operator!=(const error_category&) const noexcept;
            bool operator<(const error_category&) const noexcept;
    };

    const error_category& generic_category() noexcept;
    const error_category& system_category() noexcept;

    /**
     * 19.5.2, class error_code:
     */

    class error_code
    {
        public:
            /**
             * 19.5.2.2, constructors:
             */

            error_code() noexcept;
            error_code(int, const error_category&) noexcept;

            template<class ErrorCodeEnum>
            error_code(
                enable_if_t<is_error_code_enum_v<ErrorCodeEnum>, ErrorCodeEnum> e
            ) noexcept
            {
                val_ = static_cast<int>(e);
                cat_ = &generic_category();
            }

            /**
             * 19.5.2.3, modifiers:
             */

            void assign(int, const error_category&) noexcept;

            template<class ErrorCodeEnum>
            error_code& operator=(
                enable_if_t<is_error_code_enum_v<ErrorCodeEnum>, ErrorCodeEnum> e
            ) noexcept
            {
                val_ = static_cast<int>(e);
                cat_ = &generic_category();

                return *this;
            }

            void clear() noexcept;

            /**
             * 19.5.2.4, observers:
             */

            int value() const noexcept;
            const error_category& category() const noexcept;
            error_condition default_error_condition() const noexcept;
            string message() const;

            explicit operator bool() const noexcept
            {
                return val_ != 0;
            }

        private:
            int val_;
            const error_category* cat_;
    };

    /**
     * 19.5.2.5, non-member functions:
     */

    error_code make_error_code(errc e) noexcept;
    bool operator<(const error_code&, const error_code&) noexcept;

    template<class Char, class Traits>
    basic_ostream<Char, Traits>& operator<<(basic_ostream<Char, Traits>& os,
                                            const error_code& ec)
    {
        return os << ec.category().name() << ": " << ec.value();
    }

    /**
     * 19.5.3, class error_condition:
     */

    class error_condition
    {
        public:
            /**
             * 19.5.3.2, constructors:
             */

            error_condition() noexcept;
            error_condition(int, const error_category&) noexcept;

            template<class ErrorCodeEnum>
            error_condition(
                enable_if_t<is_error_code_enum_v<ErrorCodeEnum>, ErrorCodeEnum> e
            ) noexcept
            {
                val_ = static_cast<int>(e);
                cat_ = &generic_category();
            }

            /**
             * 19.5.3.3, modifiers:
             */

            void assign(int, const error_category&) noexcept;

            template<class ErrorCodeEnum>
            error_condition& operator=(
                enable_if_t<is_error_code_enum_v<ErrorCodeEnum>, ErrorCodeEnum> e
            ) noexcept
            {
                val_ = static_cast<int>(e);
                cat_ = &generic_category();

                return *this;
            }

            void clear() noexcept;

            /**
             * 19.5.3.4, observers:
             */

            int value() const noexcept;
            const error_category& category() const noexcept;
            string message() const;

            explicit operator bool() const noexcept
            {
                return val_ != 0;
            }

        private:
            int val_;
            const error_category* cat_;
    };

    /**
     * 19.5.3.4, non-member functions:
     */

    error_condition make_error_condition(errc e) noexcept;
    bool operator<(const error_condition&, const error_condition&) noexcept;

    /**
     * 19.5.4, comparison operators:
     */

    bool operator==(const error_code&, const error_code&) noexcept;
    bool operator==(const error_code&, const error_condition&) noexcept;
    bool operator==(const error_condition&, const error_code&) noexcept;
    bool operator==(const error_condition&, const error_condition&) noexcept;
    bool operator!=(const error_code&, const error_code&) noexcept;
    bool operator!=(const error_code&, const error_condition&) noexcept;
    bool operator!=(const error_condition&, const error_code&) noexcept;
    bool operator!=(const error_condition&, const error_condition&) noexcept;

    /**
     * 19.5.6, class system_error:
     */

    class system_error: public runtime_error
    {
        public:
            system_error(error_code, const string&);
            system_error(error_code, const char*);
            system_error(error_code);
            system_error(int, const error_category&, const string&);
            system_error(int, const error_category&, const char*);
            system_error(int, const error_category&);

            const error_code& code() const noexcept;

        private:
            error_code code_;
    };

    /**
     * 19.5.5, hash support:
     */

    template<class>
    struct hash;

    template<>
    struct hash<error_code>
    {
        size_t operator()(const error_code& ec) const noexcept
        {
            return static_cast<size_t>(ec.value());
        }

        using result_type   = size_t;
        using argument_type = error_code;
    };
}

#endif
