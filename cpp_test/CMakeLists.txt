cmake_minimum_required(VERSION 3.20)

project(alcv
    VERSION   0.1.0
    LANGUAGES CXX)

option(ALCV_BUILD_TESTS        "Build alcv unit tests"             ON)
option(ALCV_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

add_subdirectory(src)

if(ALCV_BUILD_TESTS)
    # Must be called HERE, in the top-level directory, before add_subdirectory(tests).
    # enable_testing() writes CTestTestfile.cmake into the *current* binary dir and
    # enables testing for this directory and below. ctest starts at the build root
    # and follows subdirs() from there, so if this call lived only in tests/, the
    # build root would have no CTestTestfile.cmake and ctest would report
    # "No tests were found" while still exiting 0.
    enable_testing()
    add_subdirectory(tests)
endif()
