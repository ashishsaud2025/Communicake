#pragma once

// A very small test harness. One executable per test file, no dependencies
// beyond the standard library, so the tests build anywhere OpenCV builds.

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace ctest {

struct Entry {
    std::string name;
    void (*fn)();
};

inline std::vector<Entry>& registry() {
    static std::vector<Entry> entries;
    return entries;
}

inline int& failureCount() {
    static int failures = 0;
    return failures;
}

inline int& checkCount() {
    static int checks = 0;
    return checks;
}

inline void reportFailure(const char* file, int line, const std::string& message) {
    ++failureCount();
    std::fprintf(stderr, "  FAIL %s:%d: %s\n", file, line, message.c_str());
}

inline int reg(const char* name, void (*fn)()) {
    registry().push_back(Entry{name, fn});
    return 0;
}

inline int runAll() {
    for (const Entry& entry : registry()) {
        std::printf("[ RUN  ] %s\n", entry.name.c_str());
        const int before = failureCount();
        entry.fn();
        std::printf("[ %s ] %s\n", failureCount() == before ? " OK " : "FAIL", entry.name.c_str());
    }
    std::printf("\n%d checks, %d failures\n", checkCount(), failureCount());
    return failureCount() == 0 ? 0 : 1;
}

} // namespace ctest

#define TEST(name)                                                                                 \
    static void name();                                                                            \
    static const int ctest_reg_##name = ::ctest::reg(#name, name);                                 \
    static void name()

#define CHECK(cond)                                                                                \
    do {                                                                                           \
        ++::ctest::checkCount();                                                                       \
        if (!(cond)) {                                                                             \
            ::ctest::reportFailure(__FILE__, __LINE__, #cond);                                     \
        }                                                                                          \
    } while (0)

#define CHECK_EQ(actual, expected)                                                                 \
    do {                                                                                           \
        ++::ctest::checkCount();                                                                       \
        if (!((actual) == (expected))) {                                                           \
            ::ctest::reportFailure(__FILE__, __LINE__,                                             \
                                   std::string(#actual) + " == " + #expected);                     \
        }                                                                                          \
    } while (0)

#define CHECK_NEAR(actual, expected, tolerance)                                                    \
    do {                                                                                           \
        ++::ctest::checkCount();                                                                       \
        const double ctest_a = static_cast<double>(actual);                                        \
        const double ctest_b = static_cast<double>(expected);                                      \
        if (std::fabs(ctest_a - ctest_b) > static_cast<double>(tolerance)) {                       \
            ::ctest::reportFailure(__FILE__, __LINE__, std::string(#actual) + " ~= " + #expected); \
        }                                                                                          \
    } while (0)

#define TEST_MAIN                                                                                  \
    int main() { return ::ctest::runAll(); }
