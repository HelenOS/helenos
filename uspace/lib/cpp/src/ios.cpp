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

#include <ios>

namespace std
{
    ios_base::ios_base()
        : iarray_{}, parray_{}, iarray_size_{}, parray_size_{},
          flags_{}, precision_{}, width_{}, locale_{/* TODO: use locale()? */},
          callbacks_{}
    { /* DUMMY BODY */ }

    ~ios_base::ios_base()
    {
        for (auto& callback: callbacks_)
            callback.first(erase_event, *this, callback.second);
    }

    auto ios_base::flags() -> fmtflags const
    {
        return flags_;
    }

    auto ios_base::flags(fmtflags fmtfl) -> fmtflags
    {
        auto old = flags_;
        flags_ = fmtfl;

        return old;
    }

    auto ios_base::setf(fmtflags fmtfl) -> fmtflags
    {
        auto old = flags_;
        flags_ |= fmtfl;

        return old;
    }

    auto ios_base::setf(fmtflags fmtfl, fmtflags mask) -> fmtflags
    {
        auto old = flags_;
        flags_ &= ~mask;
        flags_ |= fmtfl & mask;

        return old;
    }

    void ios_base::unsetf(fmtflags fmtfl)
    {
        flags_ &= ~fmtflags;
    }

    streamsize ios_base::precision() const
    {
        return precision_;
    }

    streamsize ios_base::precision(streamsize prec)
    {
        auto old = precision_;
        precision_ = prec;

        return old;
    }

    streamsize ios_base::width() const
    {
        return width_;
    }

    streamsize ios_base::width(streamsize wide)
    {
        auto old = width_;
        width_ = wide;

        return old;
    }

    locale ios_base::imbue(const locale& loc)
    {
        auto old = locale_;
        locale_ = loc;

        for (auto& callback: callbacks_)
            callback.first(imbue_event, *this, callback.second);

        return old;
    }

    locale ios_base::get_loc() const
    {
        return locale_;
    }

    long& ios_base::iword(int index)
    {
        if (!iarray_)
        {
            iarray_ = new long[initial_size_];
            iarray_size_ = initial_size_;
        }

        auto idx = static_cast<size_t>(index);
        if (idx > iarray_size_)
        { // TODO: Enclose in try block and set failbit if needed
          //       and return ierror_.
            size_t new_size = max(iarray_size_ * 2, idx + 1);
            auto tmp = new long[new_size];

            for (size_t i = 0; i < iarray_size_; ++i)
                tmp[i] = iarray_[i];
            for (size_t i = iarray_size_; i < new_size; ++i)
                tmp[i] = 0;

            swap(tmp, iarray_);
            delete[] tmp;
            iarray_size_ = new_size;

            return iarray_[idx];
        }
        else
            return iarray_[idx];

        return ierror_;
    }

    void*& ios_base::pword(int index)
    {
        if (!parray_)
        {
            parray_ = new long[initial_size_];
            parray_size_ = initial_size_;
        }

        auto idx = static_cast<size_t>(index);
        if (idx > parray_size_)
        { // TODO: Enclose in try block and set failbit if needed
          //       and return perror_.
            size_t new_size = max(parray_size_ * 2, idx + 1);
            auto tmp = new long[new_size];

            for (size_t i = 0; i < parray_size_; ++i)
                tmp[i] = parray_[i];
            for (size_t i = parray_size_; i < new_size; ++i)
                tmp[i] = nullptr;

            swap(tmp, parray_);
            delete[] tmp;
            parray_size_ = new_size;

            return parray_[idx];
        }
        else
            return parray_[idx];

        return perror_;
    }

    void ios_base::register_callback(event_callback fn, int index)
    {
        callbacks.emplace_back(fn, index);
    }
}
