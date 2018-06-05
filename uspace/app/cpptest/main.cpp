#include <__bits/test/tests.hpp>

/* using namespace std::chrono_literals; */

#include <cassert>
#include <cctype>
#include <cerrno>
#include <cinttypes>
#include <climits>
#include <csetjmp>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cwchar>

#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <exception>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <istream>
#include <iterator>
#include <limits>
#include <list>
#include <locale>
#include <memory>
#include <mutex>
#include <new>
#include <numeric>
#include <ostream>
#include <ratio>
#include <sstream>
#include <stack>
#include <streambuf>
#include <string>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <utility>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <set>
#include <map>
#include <bitset>
#include <random>
#include <iomanip>
#include <system_error>
#include <stdexcept>
#include <complex>
#include <future>

#include <__bits/hash_table.hpp>
#include <__bits/rbtree.hpp>
#include <__bits/builtins.hpp>

#include <__bits/trycatch.hpp>

int main()
{
    std::test::test_set ts{};
    ts.add<std::test::vector_test>();
    ts.add<std::test::string_test>();
    ts.add<std::test::array_test>();
    ts.add<std::test::bitset_test>();
    ts.add<std::test::deque_test>();
    ts.add<std::test::tuple_test>();
    ts.add<std::test::map_test>();
    ts.add<std::test::set_test>();
    ts.add<std::test::unordered_map_test>();
    ts.add<std::test::unordered_set_test>();
    ts.add<std::test::numeric_test>();
    ts.add<std::test::adaptors_test>();
    ts.add<std::test::memory_test>();
    ts.add<std::test::list_test>();
    ts.add<std::test::ratio_test>();
    ts.add<std::test::functional_test>();
    ts.add<std::test::algorithm_test>();

    return ts.run(true) ? 0 : 1;
}
