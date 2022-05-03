cmake_minimum_required(VERSION 3.14)
project(SearchServer VERSION 2.0 LANGUAGES CXX)

set (sources
    "main.cpp"
    "src/search_server.cpp"
    "src/search_server_tests.cpp"
    "src/document.cpp"
    "src/read_input_functions.cpp"
    "src/request_queue.cpp"
    "src/string_processing.cpp"
    "src/test_framework.cpp"
    "src/test_example_functions.cpp"
    "src/remove_duplicates.cpp"
    "src/process_queries.cpp"
    )

set (headers
    "include/document.h"
    "include/paginator.h"
    "include/read_input_functions.h"
    "include/request_queue.h"
    "include/search_server.h"
    "include/search_server_tests.h"
    "include/string_processing.h"
    "include/test_framework.h"
    "include/log_duration.h"
    "include/test_example_functions.h"
    "include/remove_duplicates.h"
    "include/process_queries.h"
    "include/concurrent_map.h")

add_executable(SearchServer ${sources} ${headers})

target_link_libraries(SearchServer PRIVATE -ltbb -lpthread)

target_include_directories(SearchServer PRIVATE "include")

set_target_properties(SearchServer PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

if (MSVC)
    target_compile_options(SearchServer PRIVATE /W3 /WX)
else ()
    target_compile_options(SearchServer PRIVATE
        -ltbb
        -lpthread
        #-Werror
        -Wall
        -Wextra
        -Wpedantic
        -Wcast-align
        -Wcast-qual
        -Wconversion
        -Wctor-dtor-privacy
        -Wenum-compare
        -Wfloat-equal
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Woverloaded-virtual
        -Wredundant-decls
        -Wsign-conversion
        -Wsign-promo
        -pedantic
        -pedantic-errors
        )
endif ()
