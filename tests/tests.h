#pragma once

// TODO: refactor this design or use a proper framework...

#include <string>

namespace tests {

using TestResult = std::string;
using TestFunc = void(TestResult&);
struct TestCase {
    char const* name;
    TestFunc* test;
    TestCase* next;
    static TestCase* g_tests;
    TestCase(char const* n, TestFunc f) : name(n), test(f) {
        next = g_tests;
        g_tests = this;
    }
};

}  // namespace tests

#define TEST_ARGS [[maybe_unused]] ::tests::TestResult& _test_result

#define MAKE_TEST(name)                                                        \
    static void _test_##name(TEST_ARGS);                                       \
    [[maybe_unused]] static ::tests::TestCase _test_reg_##name{#name,          \
                                                               &_test_##name}; \
    static void _test_##name([[maybe_unused]] TEST_ARGS)

#define CHECK_OP(lhs, rhs, op)                                             \
    do {                                                                   \
        auto lhs_ = lhs;                                                   \
        auto rhs_ = rhs;                                                   \
        if (lhs_ op rhs_) {                                                \
            _test_result = std::to_string(__LINE__) + ": " #lhs " (" +     \
                           std::to_string(lhs_) + ") " #op " " #rhs " (" + \
                           std::to_string(rhs_) + ")";                     \
            return;                                                        \
        }                                                                  \
    } while (false)

#define CHECK_EQ(lhs, rhs) CHECK_OP(lhs, rhs, !=)
#define CHECK_NE(lhs, rhs) CHECK_OP(lhs, rhs, ==)
#define CHECK_LT(lhs, rhs) CHECK_OP(lhs, rhs, >=)
#define CHECK_GT(lhs, rhs) CHECK_OP(lhs, rhs, <=)
#define CHECK_LE(lhs, rhs) CHECK_OP(lhs, rhs, >)
#define CHECK_GE(lhs, rhs) CHECK_OP(lhs, rhs, <)
