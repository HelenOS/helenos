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

#ifndef LIBCPP_QUEUE
#define LIBCPP_QUEUE

namespace std
{
    template<class T, class Container = deque<T>>
    class queue;

    template<
        class T, class Container = vector<T>,
        class Compare = less<typename Container::value_type>
    >
    class priority_queue;

    template<class T, class Container>
    bool operator==(const queue<T, Container>& lhs,
                    const queue<T, Container>& rhs)
    {
        return lhs.c == rhs.c;
    }

    template<class T, class Container>
    bool operator<(const queue<T, Container>& lhs,
                   const queue<T, Container>& rhs)
    {
        return lhs.c < rhs.c;
    }

    template<class T, class Container>
    bool operator!=(const queue<T, Container>& lhs,
                    const queue<T, Container>& rhs)
    {
        return lhs.c != rhs.c;
    }

    template<class T, class Container>
    bool operator>(const queue<T, Container>& lhs,
                   const queue<T, Container>& rhs)
    {
        return lhs.c > rhs.c;
    }

    template<class T, class Container>
    bool operator>=(const queue<T, Container>& lhs,
                    const queue<T, Container>& rhs)
    {
        return lhs.c >= rhs.c;
    }

    template<class T, class Container>
    bool operator<=(const queue<T, Container>& lhs,
                    const queue<T, Container>& rhs)
    {
        return lhs.c <= rhs.c;
    }

    template<class T, class Container>
    void swap(queue<T, Container>& lhs, queue<T, Container>& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

    template<class T, class Container, class Compare>
    void swap(priority_queue<T, Container, Compare>& lhs,
              priority_queue<T, Container, Compare>& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }
}

#endif
