cmake_minimum_required(VERSION 3.14)
project(CutTool CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(KCut cut.cpp)
if (WIN32 OR MSVC)
    # Need to copy clang_rt.asan_dbg_dynamic-x86_64.dll from
    # C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\<your-version>\bin\Hostx64\x64
    #target_compile_options(KWordCount_Stream PUBLIC /fsanitize=address /W4 /WX)
elseif (UNIX)
    target_compile_options(KCut PUBLIC -fsanitize=address -Wall -Werror)
    target_link_libraries(KCut PRIVATE -lasan)
endif()
