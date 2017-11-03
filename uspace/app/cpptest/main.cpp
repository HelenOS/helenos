#include <internal/test/tests.hpp>

int main()
{
    using namespace std::test;

    test_set tests{};
    tests.add<array_test>();
    tests.add<string_test>();
    tests.add<vector_test>();
    return tests.run() ? 0 : 1;
}
