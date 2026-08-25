/*
 * status_abi_test - the parts of alcv_status that must never change.
 * A failure here does not mean "fix the code". It means "do not make this change".
 */
#include "alcv/status.h"

#include "status_table.hpp"
#include "test_util.hpp"

#include <cstddef>
#include <type_traits>   /* deliberately NOT pulled into the public header */

using alcv_test::k_codes;
using alcv_test::k_count;
using alcv_test::k_names;

/* --- compile-time contract: these fail the build, not the test run --- */

static_assert(std::is_enum_v<alcv_status_t>,
              "alcv_status_t must stay an enumeration");

static_assert(std::is_same_v<std::underlying_type_t<alcv_status_t>, int>,
              "underlying type is ABI (parameter width) - must stay int");

/* std::is_scoped_enum is C++23. Implicit conversion to int is the C++17 proxy:
   it becomes false the moment someone switches to `enum class`. */
static_assert(std::is_convertible_v<alcv_status_t, int>,
              "alcv_status_t must implicitly convert to int so that `if (rc)` works");

/* Redundant given the fixed underlying type, kept as a second tripwire. */
static_assert(sizeof(alcv_status_t) == sizeof(int),
              "alcv_status_t must stay int-sized");

namespace {

/* Scenario 4. The header's static_assert already enforces this at compile time;
   this keeps the requirement visible in the test report. */
void test_ok_is_zero()
{
    CHECK(ALCV_OK == 0);
}

/* Scenario 5: static storage contract - the caller must never free the result. */
void test_pointer_is_stable_across_calls()
{
    for (std::size_t i = 0; i < k_count; ++i) {
        CHECK_MSG(alcv_status_str(k_codes[i]) == alcv_status_str(k_codes[i]),
                  "%s returned two different pointers", k_names[i]);
    }

    const alcv_status_t unknown = static_cast<alcv_status_t>(-999);
    CHECK_MSG(alcv_status_str(unknown) == alcv_status_str(unknown),
              "the fallback returned two different pointers");
}

/*
 * Wire numbers are permanently reserved. This list is HAND-WRITTEN on purpose:
 * generating it from ALCV_STATUS_LIST would make it change together with the
 * values it is supposed to pin, and it would pass forever.
 */
void test_wire_numbers_are_pinned()
{
    CHECK(ALCV_OK              ==  0);
    CHECK(ALCV_ERR_INVALID_ARG == -1);
    CHECK(ALCV_ERR_NO_MEMORY   == -2);

    /* Forcing function: adding a status code breaks this line, which is the
       prompt to add a pin above and to accept that the new number is now
       reserved for good. */
    CHECK_MSG(k_count == 3u,
              "status count changed to %zu - pin the new code above and bump this number",
              k_count);
}

}  // namespace

int main()
{
    test_ok_is_zero();
    test_pointer_is_stable_across_calls();
    test_wire_numbers_are_pinned();
    return alcv_test_report("status_abi_test");
}
