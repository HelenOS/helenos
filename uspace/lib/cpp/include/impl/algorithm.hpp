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

#ifndef LIBCPP_ALGORITHM
#define LIBCPP_ALGORITHM

#include <iterator>
#include <utility>

namespace std
{
    /**
     * 25.2, non-modyfing sequence operations:
     */

    /**
     * 25.2.1, all_of:
     */

    template<class InputIterator, class Predicate>
    bool all_of(InputIterator first, InputIterator last, Predicate pred)
    {
        while (first != last)
        {
            if (!pred(*first++))
                return false;
        }

        return true;
    }

    /**
     * 25.2.2, any_of:
     */

    template<class InputIterator, class Predicate>
    bool any_of(InputIterator first, InputIterator last, Predicate pred)
    {
        while (first != last)
        {
            if (pred(*first++))
                return true;
        }

        return false;
    }

    /**
     * 25.2.3, none_of:
     */

    template<class InputIterator, class Predicate>
    bool none_of(InputIterator first, InputIterator last, Predicate pred)
    {
        return !any_of(first, last, pred);
    }

    /**
     * 25.2.4, for_each:
     */

    // TODO: Function has to be MoveConstructible
    template<class InputIterator, class Function>
    Function for_each(InputIterator first, InputIterator last, Function f)
    {
        while (first != last)
            f(*first++);

        return move(f);
    }

    /**
     * 25.2.5, find:
     */

    template<class InputIterator, class T>
    InputIterator find(InputIterator first, InputIterator last, const T& value)
    {
        while (first != last)
        {
            if (*first == value)
                return first;
            ++first;
        }

        return last;
    }

    template<class InputIterator, class Predicate>
    InputIterator find_if(InputIterator first, InputIterator last, Predicate pred)
    {
        while (first != last)
        {
            if (pred(*first))
                return first;
            ++first;
        }

        return last;
    }

    template<class InputIterator, class Predicate>
    InputIterator find_if_not(InputIterator first, InputIterator last, Predicate pred)
    {
        while (first != last)
        {
            if (!pred(*first))
                return first;
            ++first;
        }

        return last;
    }

    /**
     * 25.2.6, find_end:
     */

    // TODO: implement

    /**
     * 25.2.7, find_first:
     */

    // TODO: implement

    /**
     * 25.2.8, adjacent_find:
     */

    template<class ForwardIterator>
    ForwardIterator adjacent_find(ForwardIterator first, ForwardIterator last)
    {
        while (first != last)
        {
            if (*first == *(first + 1))
                return first;
            ++first;
        }

        return last;
    }

    template<class ForwardIterator, class Predicate>
    ForwardIterator adjacent_find(ForwardIterator first, ForwardIterator last, Predicate pred)
    {
        while (first != last)
        {
            if (pred(*first, *(first + 1)))
                return first;
            ++first;
        }

        return last;
    }

    /**
     * 25.2.9, count:
     */

    template<class InputIterator, class T>
    typename iterator_traits<InputIterator>::difference_type
    count(InputIterator first, InputIterator last, const T& value)
    {
        typename iterator_traits<InputIterator>::difference_type cnt{};

        while (first != last)
        {
            if (*first++ == value)
                ++cnt;
        }

        return cnt;
    }

    template<class InputIterator, class Predicate>
    typename iterator_traits<InputIterator>::difference_type
    count(InputIterator first, InputIterator last, Predicate pred)
    {
        typename iterator_traits<InputIterator>::difference_type cnt{};

        while (first != last)
        {
            if (pred(*first++))
                ++cnt;
        }

        return cnt;
    }

    /**
     * 25.2.10, mismatch:
     */

    template<class InputIterator1, class InputIterator2>
    pair<InputIterator1, InputIterator2> mismatch(InputIterator1 first1, InputIterator1 last1,
                                                  InputIterator2 first2)
    {
        while (first1 != last1 && *first1++ == *first2++)
        { /* DUMMY BODY */ }

        return make_pair(first1, first2);
    }

    template<class InputIterator1, class InputIterator2, class BinaryPredicate>
    pair<InputIterator1, InputIterator2> mismatch(InputIterator1 first1, InputIterator1 last1,
                                                  InputIterator2 first2, BinaryPredicate pred)
    {
        while (first1 != last1 && pred(*first1++, *first2++))
        { /* DUMMY BODY */ }

        return make_pair(first1, first2);
    }

    template<class InputIterator1, class InputIterator2>
    pair<InputIterator1, InputIterator2> mismatch(InputIterator1 first1, InputIterator1 last1,
                                                  InputIterator2 first2, InputIterator2 last2)
    {
        while (first1 != last1 && first2 != last2 && *first1++ == *first2++)
        { /* DUMMY BODY */ }

        return make_pair(first1, first2);
    }

    template<class InputIterator1, class InputIterator2, class BinaryPredicate>
    pair<InputIterator1, InputIterator2> mismatch(InputIterator1 first1, InputIterator1 last1,
                                                  InputIterator2 first2, InputIterator2 last2,
                                                  BinaryPredicate pred)
    {
        while (first1 != last1 && first2 != last2 && pred(*first1++, *first2++))
        { /* DUMMY BODY */ }

        return make_pair(first1, first2);
    }

    /**
     * 25.2.11, equal:
     */

    template<class InputIterator1, class InputIterator2>
    bool equal(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2)
    {
        auto last2 = first2 + (last1 - first1);

        return equal(first1, last1, first2, last2);
    }

    template<class InputIterator1, class InputIterator2>
    bool equal(InputIterator1 first1, InputIterator1 last1,
               InputIterator2 first2, InputIterator2 last2)
    {
        if ((last1 - first1) != (last2 - first2))
            return false;

        while (first1 != last1)
        {
            if (*first1++ != *first2++)
                return false;
        }

        return true;
    }

    template<class InputIterator1, class InputIterator2, class BinaryPredicate>
    bool equal(InputIterator1 first1, InputIterator1 last1,
               InputIterator2 first2, BinaryPredicate pred)
    {
        auto last2 = first2 + (last1 - first1);

        return equal(first1, last1, first2, last2, pred);
    }

    template<class InputIterator1, class InputIterator2, class BinaryPredicate>
    bool equal(InputIterator1 first1, InputIterator1 last1,
               InputIterator2 first2, InputIterator2 last2,
               BinaryPredicate pred)
    {
        if ((last1 - first1) != (last2 - first2))
            return false;

        while (first1 != last1)
        {
            if (!pred(*first1++, *first2++))
                return false;
        }

        return true;
    }

    /**
     * 25.2.12, is_permutation:
     */

    // TODO: implement

    /**
     * 25.2.13, search:
     */

    // TODO: implement

    /**
     * 25.3, mutating sequence operations:
     */

    /**
     * 25.3.1, copy:
     */

    template<class InputIterator, class OutputIterator>
    OutputIterator copy(InputIterator first, InputIterator last, OutputIterator result)
    {
        while (first != last)
            *result++ = *first++;

        return result;
    }

    template<class InputIterator, class Size, class OutputIterator>
    OutputIterator copy_n(InputIterator first, Size count, OutputIterator result)
    {
        for (Size i = 0; i < count; ++i, ++first, ++result)
            *result = *first;

        return result;
    }

    template<class InputIterator, class OutputIterator, class Predicate>
    OutputIterator copy_if(InputIterator first, InputIterator last,
                           OutputIterator result, Predicate pred)
    {
        while (first != last)
        {
            if (pred(*first))
                *result++ = *first;
            ++first;
        }

        return result;
    }

    template<class BidirectionalIterator1, class BidirectionalIterator2>
    BidirectionalIterator2 copy_backward(BidirectionalIterator1 first, BidirectionalIterator1 last,
                                         BidirectionalIterator2 result)
    {
        // Note: We're copying [first, last) so we need to skip the initial value of last.
        while (last-- != first)
            *result-- = *last;

        return result;
    }

    /**
     * 25.3.2, move:
     */

    template<class InputIterator, class OutputIterator>
    OutputIterator move(InputIterator first, InputIterator last, OutputIterator result)
    {
        while (first != last)
            *result++ = move(first++);

        return result;
    }

    template<class BidirectionalIterator1, class BidirectionalIterator2>
    BidirectionalIterator2 move_backward(BidirectionalIterator1 first, BidirectionalIterator1 last,
                                         BidirectionalIterator2 result)
    {
        // Note: We're copying [first, last) so we need to skip the initial value of last.
        while (last-- != first)
            *result-- = move(*last);
    }

    /**
     * 25.3.3, swap:
     */

    template<class ForwardIterator1, class ForwardIterator2>
    ForwardIterator2 swap_ranges(ForwardIterator1 first1, ForwardIterator1 last1,
                                 ForwardIterator2 first2)
    {
        while (first1 != last1)
            swap(*first1++, *first2++);

        return first2;
    }

    template<class ForwardIterator1, class ForwardIterator2>
    void iter_swap(ForwardIterator1 iter1, ForwardIterator2 iter2)
    {
        swap(*iter1, *iter2);
    }

    /**
     * 25.3.4, transform:
     */

    template<class InputIterator, class OutputIterator, class UnaryOperation>
    OutputIterator transform(InputIterator first, InputIterator last,
                             OutputIterator result, UnaryOperation op)
    {
        while (first != last)
            *result++ = op(*first++);
    }

    template<class InputIterator1, class InputIterator2,
             class OutputIterator, class BinaryOperation>
    OutputIterator transform(InputIterator1 first1, InputIterator1 last1,
                             InputIterator2 first2, OutputIterator result,
                             BinaryOperation op)
    {
        while (first1 != last1)
            *result++ = op(*first1++, *first2++);
    }

    /**
     * 25.3.5, replace:
     */

    template<class ForwardIterator, class T>
    void replace(ForwardIterator first, ForwardIterator last,
                 const T& old_value, const T& new_value)
    {
        while (first != last)
        {
            if (*first == old_value)
                *first = new_value;
            ++first;
        }
    }

    template<class ForwardIterator, class Predicate, class T>
    void replace_if(ForwardIterator first, ForwardIterator last,
                    Predicate pred, const T& new_value)
    {
        while (first != last)
        {
            if (pred(*first))
                *first = new_value;
            ++first;
        }
    }

    template<class InputIterator, class OutputIterator, class T>
    OutputIterator replace_copy(InputIterator first, InputIterator last,
                                OutputIterator result, const T& old_value,
                                const T& new_value)
    {
        while (first != last)
        {
            if (*first == old_value)
                *result = new_value;
            else
                *result = *first;

            ++first;
            ++result;
        }
    }

    template<class InputIterator, class OutputIterator, class Predicate, class T>
    OutputIterator replace_copy_if(InputIterator first, InputIterator last,
                                OutputIterator result, Predicate pred,
                                const T& new_value)
    {
        while (first != last)
        {
            if (pred(*first))
                *result = new_value;
            else
                *result = *first;

            ++first;
            ++result;
        }
    }

    /**
     * 25.3.6, fill:
     */

    template<class ForwardIterator, class T>
    void fill(ForwardIterator first, ForwardIterator last, const T& value)
    {
        while (first != last)
            *first++ = value;
    }

    template<class InputIterator, class Size, class T>
    void fill_n(InputIterator first, Size count, const T& value)
    {
        for (Size i = 0; i < count; ++i)
            *first++ = value;
    }

    /**
     * 25.3.7, generate:
     */

    template<class ForwardIterator, class Generator>
    void generate(ForwardIterator first, ForwardIterator last,
                  Generator gen)
    {
        while (first != last)
            *first++ = gen();
    }

    template<class OutputIterator, class Size, class Generator>
    void generate(OutputIterator first, Size count, Generator gen)
    {
        for (Size i = 0; i < count; ++i)
            *first++ = gen();
    }

    /**
     * 25.3.8, remove:
     */

    template<class ForwardIterator, class T>
    ForwardIterator remove(ForwardIterator first, ForwardIterator last,
                           const T& value)
    {
        auto it = first;
        while (it != last)
        {
            if (*it != value)
                *first++ = move(*it);
        }

        return first;
    }

    template<class ForwardIterator, class Predicate>
    ForwardIterator remove_if(ForwardIterator first, ForwardIterator last,
                              Predicate pred)
    {
        auto it = first;
        while (it != last)
        {
            if (!pred(*it))
                *first++ = move(*it);
        }

        return first;
    }

    template<class InputIterator, class OutputIterator, class T>
    OutputIterator remove_copy(InputIterator first, InputIterator last,
                               OutputIterator result, const T& value)
    {
        while (first != last)
        {
            if (*first != value)
                *result++ = *first;
            ++first;
        }

        return result;
    }

    template<class InputIterator, class OutputIterator, class Predicate>
    OutputIterator remove_copy_if(InputIterator first, InputIterator last,
                                  OutputIterator result, Predicate pred)
    {
        while (first != last)
        {
            if (!pred(*first))
                *result++ = *first;
            ++first;
        }

        return result;
    }

    /**
     * 25.3.9, unique:
     */

    // TODO: implement

    /**
     * 25.3.10, reverse:
     */

    template<class BidirectionalIterator>
    void reverse(BidirectionalIterator first, BidirectionalIterator last)
    {
        if (first == last)
            return;
        auto mid_count = (last - first) / 2;

        --last;
        for (decltype(mid_count) i = 0; i < mid_count; ++i)
            iter_swap(first++, last--);
    }

    template<class BidirectionalIterator, class OutputIterator>
    OutputIterator reverse_copy(BidirectionalIterator first,
                                BidirectionalIterator last,
                                OutputIterator result)
    {
        while (--last != first)
            *result++ = *last;
    }

    /**
     * 25.3.11, rotate:
     */

    // TODO: implement

    /**
     * 25.3.12, shuffle:
     */

    // TODO: implement

    /**
     * 25.3.13, partitions:
     */

    // TODO: implement

    /**
     * 25.4, sorting and related operations:
     */

    /**
     * 25.4.1, sorting:
     */

    /**
     * 25.4.1.1, sort:
     */

    // TODO: implement

    /**
     * 25.4.1.2, stable_sort:
     */

    // TODO: implement

    /**
     * 25.4.1.3, partial_sort:
     */

    // TODO: implement

    /**
     * 25.4.1.4, partial_sort_copy:
     */

    // TODO: implement

    /**
     * 25.4.1.5, is_sorted:
     */

    template<class ForwardIterator>
    bool is_sorted(ForwardIterator first, ForwardIterator last)
    {
        return is_sorted_until(first, last) == last;
    }

    template<class ForwardIterator, class Comp>
    bool is_sorted(ForwardIterator first, ForwardIterator last,
                   Comp comp)
    {
        return is_sorted_until(first, last, comp) == last;
    }

    template<class ForwardIterator>
    ForwardIterator is_sorted_until(ForwardIterator first, ForwardIterator last)
    {
        if (distance(first, last) < 2)
            return last;

        while (first != last)
        {
            if (*first > *(++first))
                return first;
        }

        return last;
    }

    template<class ForwardIterator, class Comp>
    ForwardIterator is_sorted_until(ForwardIterator first, ForwardIterator last,
                                    Comp comp)
    {
        if (distance(first, last) < 2)
            return last;

        while (first != last)
        {
            if (!comp(*first, *(++first)))
                return first;
        }

        return last;
    }

    /**
     * 25.4.2, nth_element:
     */

    // TODO: implement

    /**
     * 25.4.3, binary search:
     */

    /**
     * 25.4.3.1, lower_bound
     */

    // TODO: implement

    /**
     * 25.4.3.2, upper_bound
     */

    // TODO: implement

    /**
     * 25.4.3.3, equal_range:
     */

    // TODO: implement

    /**
     * 25.4.3.4, binary_search:
     */

    // TODO: implement

    /**
     * 25.4.4, merge:
     */

    // TODO: implement

    /**
     * 25.4.5, set operations on sorted structures:
     */

    /**
     * 25.4.5.1, includes:
     */

    // TODO: implement

    /**
     * 25.4.5.2, set_union:
     */

    // TODO: implement

    /**
     * 25.4.5.3, set_intersection:
     */

    // TODO: implement

    /**
     * 25.4.5.4, set_difference:
     */

    // TODO: implement

    /**
     * 25.4.5.5, set_symmetric_difference:
     */

    // TODO: implement

    /**
     * 25.4.6, heap operations:
     */

    /**
     * 25.4.6.1, push_heap:
     */

    // TODO: implement

    /**
     * 25.4.6.2, pop_heap:
     */

    // TODO: implement

    /**
     * 25.4.6.3, make_heap:
     */

    // TODO: implement

    /**
     * 25.4.6.4, sort_heap:
     */

    // TODO: implement

    /**
     * 25.4.6.5, is_heap:
     */

    // TODO: implement

    /**
     * 25.4.7, minimum and maximum:
     * // TODO: implement container versions when we have
     *          numeric limits and min/max element
     * // TODO: versions with comparators
     * // TODO: minmax
     */

    template<class T>
    constexpr const T& min(const T& lhs, const T& rhs)
    {
        return (lhs < rhs) ? lhs : rhs;
    }

    template<class T>
    constexpr const T& max(const T& lhs, const T& rhs)
    {
        return (lhs > rhs) ? lhs : rhs;
    }

    /**
     * 25.4.8, lexicographical comparison:
     */

    // TODO: implement

    /**
     * 25.4.9, permutation generators:
     */

    // TODO: implement
}

#endif
