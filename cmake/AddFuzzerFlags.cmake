macro(add_fuzzer_flags)
    find_package(Threads REQUIRED)
    string(REPLACE "-l" "" PTHREAD_LIB_ONLY "${CMAKE_THREAD_LIBS_INIT}")
    set(CMAKE_REQUIRED_LIBRARIES ${LIB_FUZZER} ${PTHREAD_LIB_ONLY} stdc++)
    set(CMAKE_REQUIRED_FLAGS "-fsanitize=address")

    include(AddCompilerFlags)
    add_compiler_flags(
        -fsanitize=address
        -fsanitize-coverage=trace-pc-guard
    )

    enable_language(CXX)
    include(AddCxxCompilerFlags)
    add_cxx_compiler_flags(
        -fsanitize=address
        -fsanitize-coverage=trace-pc-guard
    )
endmacro()
