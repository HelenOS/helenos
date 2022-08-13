/*
 * SPDX-FileCopyrightText: 2018 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <__bits/io/streambufs.hpp>
#include <ios>
#include <iostream>
#include <new>

namespace std
{
    istream cin{nullptr};
    ostream cout{nullptr};

    namespace aux
    {
        ios_base::Init init{};
    }

    int ios_base::Init::init_cnt_{};

    ios_base::Init::Init()
    {
        if (init_cnt_++ == 0)
        {
            // TODO: These buffers should be static too
            //       in case somebody reassigns to cout/cin.
            ::new(&cin) istream{::new aux::stdin_streambuf<char>{}};
            ::new(&cout) ostream{::new aux::stdout_streambuf<char>{}};

            cin.tie(&cout);
        }
    }

    ios_base::Init::~Init()
    {
        if (--init_cnt_ == 0)
            cout.flush();
    }
}
