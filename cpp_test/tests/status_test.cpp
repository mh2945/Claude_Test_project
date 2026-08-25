/*
 * status_test - behaviour of alcv_status_str().
 * Scenarios 1, 2, 3. Contract/ABI checks live in status_abi_test.cpp.
 */
#include "alcv/status.h"

#include "status_table.hpp"
#include "test_util.hpp"

#include <cstddef>
#include <string_view>

using alcv_test::k_codes;
using alcv_test::k_count;
using alcv_test::k_names;

namespace {

/* Scenario 1: every defined code maps to a non-null, non-empty message. */
void test_every_code_has_a_message()
{
    for (std::size_t i = 0; i < k_count; ++i) {
        const char *msg = alcv_status_str(k_codes[i]);

        CHECK_MSG(msg != nullptr, "%s returned nullptr", k_names[i]);
        if (msg == nullptr) {
            continue;  /* keep going: report every code, not just the first bad one */
        }
        CHECK_MSG(msg[0] != '\0', "%s returned an empty string", k_names[i]);
    }
}

/*
 * Scenario 2: no two codes share a message (catches copy-paste in the list).
 * Compares what alcv_status_str() RETURNS, not the msg column of the X-macro,
 * so the whole mapping path is under test.
 */
void test_messages_are_unique()
{
    for (std::size_t i = 0; i < k_count; ++i) {
        for (std::size_t j = i + 1; j < k_count; ++j) {
            const char *a = alcv_status_str(k_codes[i]);
            const char *b = alcv_status_str(k_codes[j]);

            /* string_view compares contents. Comparing const char* with == would
               compare addresses and pass by accident. */
            CHECK_MSG(std::string_view(a) != std::string_view(b),
                      "%s and %s share the message \"%s\"", k_names[i], k_names[j], a);
        }
    }
}

/* Scenario 3: values that are not in the list still get a usable fallback. */
void test_unknown_codes_get_a_fallback()
{
    /* Well-defined because alcv_status has a fixed underlying type of int. */
    constexpr alcv_status_t undefined[] = {
        static_cast<alcv_status_t>(-999),
        static_cast<alcv_status_t>(12345),
    };

    for (const alcv_status_t s : undefined) {
        const int raw = static_cast<int>(s);
        const char *msg = alcv_status_str(s);

        CHECK_MSG(msg != nullptr, "code %d returned nullptr", raw);
        if (msg == nullptr) {
            continue;
        }
        CHECK_MSG(msg[0] != '\0', "code %d returned an empty string", raw);

        /* The fallback must not be mistakable for a real status message. */
        for (std::size_t i = 0; i < k_count; ++i) {
            CHECK_MSG(std::string_view(msg) != std::string_view(alcv_status_str(k_codes[i])),
                      "fallback for %d collides with %s (\"%s\")", raw, k_names[i], msg);
        }
    }
}

}  // namespace

int main()
{
    test_every_code_has_a_message();
    test_messages_are_unique();
    test_unknown_codes_get_a_fallback();
    return alcv_test_report("status_test");
}
