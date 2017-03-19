find_library(LIB_FUZZING_ENGINE FuzzingEngine)
if (NOT LIB_FUZZING_ENGINE)
    find_library(LIB_FUZZER Fuzzer)
    if (NOT LIB_FUZZER)
        message(FATAL_ERROR "Fuzzer not found")
    endif()

    set(LIB_FUZZING_ENGINE ${LIB_FUZZER})
endif()

add_executable(fuzzer main.cpp)
target_link_libraries(fuzzer
    ${LIB_FUZZING_ENGINE}
    event_core_static
    event_pthreads_static)

# For local fuzzing
add_custom_target(fuzz
    fuzzer -runs=100000 -print_coverage=1 -dump_coverage=1
    DEPENDS fuzzer)
