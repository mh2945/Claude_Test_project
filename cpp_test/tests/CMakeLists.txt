# Two executables, one per concern:
#   status_test     - behaviour. A failure means "fix the code".
#   status_abi_test - contract.  A failure means "do not make this change".
foreach(t IN ITEMS status_test status_abi_test)
    add_executable(${t} ${t}.cpp)

    # PRIVATE alcv::alcv - the tests consume the library, nothing consumes the tests.
    target_link_libraries(${t} PRIVATE alcv::alcv)

    # The tests use std::size / std::string_view / inline variables, so they need
    # C++17 themselves. alcv only exports cxx_std_11 through its PUBLIC interface,
    # which is exactly the point: dropping this line breaks the build.
    target_compile_features(${t} PRIVATE cxx_std_17)

    target_compile_options(${t} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>)

    # add_test only REGISTERS name -> command; it does not build anything.
    # cmake --build must run before ctest.
    add_test(NAME ${t} COMMAND ${t})
endforeach()
