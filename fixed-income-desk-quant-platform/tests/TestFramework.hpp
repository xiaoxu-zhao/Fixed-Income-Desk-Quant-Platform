#pragma once

#ifdef FIQ_USE_CATCH2

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#define REQUIRE_APPROX(actual, expected, tolerance) REQUIRE((actual) == Catch::Approx(expected).margin(tolerance))

#else

#include <cmath>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace test {

struct TestCase {
    std::string name;
    std::function<void()> body;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar {
    Registrar(std::string name, std::function<void()> body) {
        registry().push_back({std::move(name), std::move(body)});
    }
};

inline void require(bool condition, const char* expression, const char* file, int line) {
    if (!condition) {
        std::ostringstream out;
        out << file << ":" << line << " REQUIRE failed: " << expression;
        throw std::runtime_error(out.str());
    }
}

inline void requireApprox(double actual,
                          double expected,
                          double tolerance,
                          const char* actualExpression,
                          const char* expectedExpression,
                          const char* file,
                          int line) {
    if (std::fabs(actual - expected) > tolerance) {
        std::ostringstream out;
        out << file << ":" << line << " REQUIRE_APPROX failed: "
            << actualExpression << "=" << actual << " "
            << expectedExpression << "=" << expected
            << " tolerance=" << tolerance;
        throw std::runtime_error(out.str());
    }
}

inline int runAll() {
    int failures = 0;
    for (const auto& testCase : registry()) {
        try {
            testCase.body();
            std::cout << "[PASS] " << testCase.name << "\n";
        } catch (const std::exception& ex) {
            ++failures;
            std::cerr << "[FAIL] " << testCase.name << "\n  " << ex.what() << "\n";
        }
    }
    std::cout << registry().size() << " tests, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}

} // namespace test

#define TEST_CONCAT_IMPL(x, y) x##y
#define TEST_CONCAT(x, y) TEST_CONCAT_IMPL(x, y)
#define TEST_CASE(name) \
    static void TEST_CONCAT(test_body_, __LINE__)(); \
    static test::Registrar TEST_CONCAT(test_registrar_, __LINE__)(name, TEST_CONCAT(test_body_, __LINE__)); \
    static void TEST_CONCAT(test_body_, __LINE__)()

#define REQUIRE(expr) test::require(static_cast<bool>(expr), #expr, __FILE__, __LINE__)
#define REQUIRE_APPROX(actual, expected, tolerance) \
    test::requireApprox((actual), (expected), (tolerance), #actual, #expected, __FILE__, __LINE__)

#endif
