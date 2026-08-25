#include "alcv/status.h"

const char *alcv_status_str(alcv_status_t s) noexcept
{
    switch (s) {
#define ALCV_STATUS_CASE(name, value, msg) case name: return msg;
    ALCV_STATUS_LIST(ALCV_STATUS_CASE)
#undef ALCV_STATUS_CASE
    }

    /*
     * Deliberately no `default:` above. With a default label the compiler stops
     * reporting missing enumerators (-Wswitch), which is exactly the mistake we
     * are trying to make impossible. Out-of-range integers fall through to here.
     *
     * This must stay a string literal. Formatting the number into a static
     * buffer would break thread safety, break the "same code -> same pointer"
     * contract, and let one call clobber the previous call's result.
     */
    return "unknown status";
}
