# Test program
add_executable(tests
    tests.cc
    test_bandpass.cc
    test_lowpass.cc
)
target_link_libraries(tests PUBLIC vocoder)

add_test(NAME tests COMMAND tests)
