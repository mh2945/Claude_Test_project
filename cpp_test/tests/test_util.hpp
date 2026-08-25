#ifndef ALCV_TEST_UTIL_HPP
#define ALCV_TEST_UTIL_HPP

#include <cstdio>

/*
 * C++17 inline variable: every translation unit that includes this header
 * refers to the SAME object. A plain `static int` would give each TU its own
 * copy, so splitting a suite across files would silently split the counter too.
 */
inline int g_failures = 0;

/* Checks do not abort, so one run reports every failure instead of just the first. */
#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::fprintf(stderr, "FAIL %s:%d: %s\n",                           \
                         __FILE__, __LINE__, #cond);                           \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

#define CHECK_MSG(cond, ...)                                                   \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::fprintf(stderr, "FAIL %s:%d: %s\n      ",                     \
                         __FILE__, __LINE__, #cond);                           \
            std::fprintf(stderr, __VA_ARGS__);                                 \
            std::fputc('\n', stderr);                                          \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

/* ctest judges a test purely by the process exit code. */
inline int alcv_test_report(const char *suite)
{
    if (g_failures == 0) {
        std::printf("PASS %s\n", suite);
        return 0;
    }
    std::fprintf(stderr, "FAILED %s: %d check(s) failed\n", suite, g_failures);
    return 1;
}

#endif /* ALCV_TEST_UTIL_HPP */
