add_library(alcv STATIC
    status.cpp)

# Namespaced alias so consumers write alcv::alcv whether they got us through
# add_subdirectory() or find_package(). A misspelled name containing "::" is a
# configure-time error instead of a link-time one.
add_library(alcv::alcv ALIAS alcv)

# PUBLIC: include/alcv/status.h is part of the interface, so anything linking
# alcv must be able to find it.
target_include_directories(alcv PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

# The split matters:
#   PUBLIC  cxx_std_11 - what the HEADER actually needs (enum : int, static_assert,
#                        using alias). Requiring 17 here would force C++17 on every
#                        consumer for no reason.
#   PRIVATE cxx_std_17 - what OUR .cpp compiles as. CMake takes the max (17) for
#                        this target and exports only 11 to consumers.
# Safe because no standard-library type crosses the interface (const char* + enum).
# If std::string / std::optional ever appear in the public header, raise the
# PUBLIC requirement to match.
target_compile_features(alcv
    PUBLIC  cxx_std_11
    PRIVATE cxx_std_17)

# PRIVATE: our warning policy is about compiling our sources. Leaking -Werror
# into a consumer's build would break them on a compiler upgrade.
#
#   -Wswitch                 (in -Wall) missing enumerator, only when no default:
#   -Wswitch-enum            missing enumerator EVEN IF a default: exists
#   -Wcovered-switch-default (clang) a default: on an already-exhaustive switch
target_compile_options(alcv PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra;-Wswitch-enum>
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wcovered-switch-default>
    $<$<CXX_COMPILER_ID:MSVC>:/W4;/w14061;/w14062>)

if(ALCV_WARNINGS_AS_ERRORS)
    target_compile_options(alcv PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Werror>
        $<$<CXX_COMPILER_ID:MSVC>:/WX>)
endif()
