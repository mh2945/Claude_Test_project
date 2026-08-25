#ifndef ALCV_STATUS_H
#define ALCV_STATUS_H

/*
 * alcv status codes.
 *
 * ABI CONTRACT — read before touching ALCV_STATUS_LIST:
 *   - A number that has shipped is reserved forever.
 *   - Never change the value of an existing entry.
 *   - Never delete an entry.
 *   - Never reuse a number that once belonged to a different entry.
 *   - New codes are appended at the end with the next unused negative value.
 *
 * The message lives on the same line as the code, so "added a code but forgot
 * the string" cannot happen: there is exactly one place to edit.
 */
#define ALCV_STATUS_LIST(X)                             \
    X(ALCV_OK,              0,  "success")              \
    X(ALCV_ERR_INVALID_ARG, -1, "invalid argument")     \
    X(ALCV_ERR_NO_MEMORY,   -2, "out of memory")

/*
 * `: int` is a fixed underlying type (C++11). Two things depend on it:
 *   - static_cast<alcv_status_t>(-999) is well-defined; the value range of the
 *     enumeration is the range of int, not just the range of the enumerators.
 *   - sizeof(alcv_status_t) == sizeof(int) is guaranteed by the standard, so
 *     the parameter-passing width is stable across the ABI boundary.
 * Unscoped (not `enum class`) on purpose: `if (rc)` must keep working.
 */
enum alcv_status : int {
#define ALCV_STATUS_ENUM_ENTRY(name, value, msg) name = (value),
    ALCV_STATUS_LIST(ALCV_STATUS_ENUM_ENTRY)
#undef ALCV_STATUS_ENUM_ENTRY
};

using alcv_status_t = alcv_status;

/* Breaks at the consumer's include site, not merely in our test suite. */
static_assert(ALCV_OK == 0, "ALCV_OK must stay 0 so that `if (rc)` keeps working");

/*
 * Return a human-readable English message for a status code.
 *
 *   - Never returns nullptr. Values outside the list get a fallback string.
 *   - The returned pointer has static storage duration: never free/delete it,
 *     and it stays valid for the lifetime of the program.
 *   - Calling twice with the same code yields the identical pointer.
 *
 * `noexcept` is part of the function type since C++17 (P0012R1); removing it
 * later would be an ABI break for anyone storing a pointer to this function.
 */
const char *alcv_status_str(alcv_status_t s) noexcept;

#endif /* ALCV_STATUS_H */
