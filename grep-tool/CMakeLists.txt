cmake_minimum_required(VERSION 3.14)
project(GrepTool CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(regex-engine)

add_executable(KGrep grep.cpp)
target_include_directories(KGrep PRIVATE regex-engine/include)
target_link_libraries(KGrep PRIVATE KRegexEngine)

if (WIN32 OR MSVC)
    # Need to copy clang_rt.asan_dbg_dynamic-x86_64.dll from
    # C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\<your-version>\bin\Hostx64\x64
    #target_compile_options(KGrep PUBLIC /fsanitize=address /W4 /WX)
elseif (UNIX)
    # target_compile_options(KGrep PUBLIC -fsanitize=address -Wall -Werror)
    # target_link_libraries(KGrep PRIVATE -lasan)
endif()