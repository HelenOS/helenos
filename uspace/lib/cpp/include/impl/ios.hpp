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

#ifndef LIBCPP_IOS
#define LIBCPP_IOS

#include <cstdlib>
#include <locale>
#include <system_error>

namespace std
{
    using streamoff = long long;
    using streamsize = ssize_t;

    /**
     * 27.5.3, ios_base:
     */

    class ios_base
    {
        public:
            virtual ~ios_base();

            ios_base(const ios_base&) = delete;
            ios_base& operator=(const ios_base&) = delete;

            /**
             * 27.5.3.1.1, failure:
             */

            class failure: public system_error
            {
                // TODO: implement
            };

            /**
             * 27.5.3.1.2, fmtflags:
             */

            using fmtflags = uint32_t;
            static constexpr fmtflags boolalpha   = 0b00'0000'0000'0000'0001;
            static constexpr fmtflags dec         = 0b00'0000'0000'0000'0010;
            static constexpr fmtflags fixed       = 0b00'0000'0000'0000'0100;
            static constexpr fmtflags hex         = 0b00'0000'0000'0000'1000;
            static constexpr fmtflags internal    = 0b00'0000'0000'0001'0000;
            static constexpr fmtflags left        = 0b00'0000'0000'0010'0000;
            static constexpr fmtflags oct         = 0b00'0000'0000'0100'0000;
            static constexpr fmtflags right       = 0b00'0000'0000'1000'0000;
            static constexpr fmtflags scientific  = 0b00'0000'0001'0000'0000;
            static constexpr fmtflags showbase    = 0b00'0000'0010'0000'0000;
            static constexpr fmtflags showpoint   = 0b00'0000'0100'0000'0000;
            static constexpr fmtflags showpos     = 0b00'0000'1000'0000'0000;
            static constexpr fmtflags skipws      = 0b00'0001'0000'0000'0000;
            static constexpr fmtflags unitbuf     = 0b00'0010'0000'0000'0000;
            static constexpr fmtflags uppercase   = 0b00'0100'0000'0000'0000;
            static constexpr fmtflags adjustfield = 0b00'1000'0000'0000'0000;
            static constexpr fmtflags basefield   = 0b01'0000'0000'0000'0000;
            static constexpr fmtflags floatfield  = 0b10'0000'0000'0000'0000;

            /**
             * 27.5.3.1.3, iostate:
             */

            using iostate = uint8_t;
            static constexpr iostate badbit  = 0b0001;
            static constexpr iostate eofbit  = 0b0010;
            static constexpr iostate failbit = 0b0100;
            static constexpr iostate goodbit = 0b1000;

            /**
             * 27.5.3.1.4, openmode:
             */

            using openmode = uint8_t;
            static constexpr openmode app    = 0b00'0001;
            static constexpr openmode ate    = 0b00'0010;
            static constexpr openmode binary = 0b00'0100;
            static constexpr openmode in     = 0b00'1000;
            static constexpr openmode out    = 0b01'0000;
            static constexpr openmode trunc  = 0b10'0000;

            /**
             * 27.5.3.1.5, seekdir:
             */

            using seekdir = uint8_t;
            static constexpr seekdir beg = 0b001;
            static constexpr seekdir cur = 0b010;
            static constexpr seekdir end = 0b100;

            /**
             * 27.5.3.1.6, class Init:
             */

            class Init
            {
                // TODO: implement
            };

            /**
             * 27.5.3.2, fmtflags state:
             */

            fmtflags flags() const;
            fmtflags flags(fmtflags fmtfl);
            fmtflags setf(fmtflags fmtfl);
            fmtflags setf(fmtflags fmtfl, fmtflags mask);
            void unsetf(fmtflags mask);

            streamsize precision() const;
            streamsize precision(streamsize prec);
            streamsize width() const;
            streamsize width(streamsize wide);

            /**
             * 27.5.3.3, locales:
             */

            locale imbue(const locale& loc);
            locale getloc() const;

            /**
             * 27.5.3.5, storage:
             */

            static int xalloc();
            long& iword(int index);
            void*& pword(int index);

            /**
             * 27.5.3.6, callbacks:
             */

            enum event
            {
                erase_event,
                imbue_event,
                copyfmt_event
            };

            using event_callback = void (*)(event, ios_base&, int);
            void register_callback(event_callback fn, int index);

            static bool sync_with_stdio(bool sync = true);

        protected:
            ios_base();

        private:
            static int index_{};

            long* iarray_;
            void** parray_;
    };
}

#endif
