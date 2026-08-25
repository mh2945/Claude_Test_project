#ifndef ALCV_TEST_STATUS_TABLE_HPP
#define ALCV_TEST_STATUS_TABLE_HPP

#include "alcv/status.h"

#include <cstddef>
#include <iterator>   /* std::size (C++17) */

/*
 * The same X-macro list, expanded a third way: into tables the tests can walk.
 * Adding a status code therefore extends the test coverage automatically.
 */
namespace alcv_test {

constexpr alcv_status_t k_codes[] = {
#define X(name, value, msg) name,
    ALCV_STATUS_LIST(X)
#undef X
};

/* Stringified enumerator names, so failure output says ALCV_ERR_NO_MEMORY, not -2. */
constexpr const char *k_names[] = {
#define X(name, value, msg) #name,
    ALCV_STATUS_LIST(X)
#undef X
};

constexpr std::size_t k_count = std::size(k_codes);

static_assert(k_count == std::size(k_names), "code table and name table are out of sync");

}  // namespace alcv_test

#endif /* ALCV_TEST_STATUS_TABLE_HPP */
