cmake_minimum_required(VERSION 3.14)
project(RegexEngine CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(KREGEX_ENGINE_BUILD_TESTS "Build tests" ON)

add_library(KRegexEngine STATIC src/parser.cpp src/nfa.cpp src/matcher.cpp)

target_include_directories(KRegexEngine PUBLIC include)
if (WIN32 OR MSVC)
    # Need to copy clang_rt.asan_dbg_dynamic-x86_64.dll from
    # C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\<your-version>\bin\Hostx64\x64
    #target_compile_options(KRegexEngine PUBLIC /fsanitize=address /W4 /WX)
elseif (UNIX)
    target_compile_options(KRegexEngine PUBLIC -fsanitize=address -Wall -Werror)
    target_link_libraries(KRegexEngine PRIVATE -lasan)
endif()

if (KREGEX_ENGINE_BUILD_TESTS)
    enable_testing() # Enable CTest support
    add_subdirectory(test)
endif()