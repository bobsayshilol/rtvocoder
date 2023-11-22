#include "tests.h"

#include <cstdio>
#include <cstdlib>
#include <utility>

namespace tests {
TestCase* TestCase::g_tests;
}

int main() {
    // Run the tests
    bool success = true;
    for (auto* test = tests::TestCase::g_tests; test != nullptr;
         test = test->next) {
        std::string error;
        test->test(error);
        if (error.empty()) {
            printf("Test %s: passed\n", test->name);
        } else {
            printf("Test %s: FAILED: %s\n", test->name, error.c_str());
            success = false;
        }
    }
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
