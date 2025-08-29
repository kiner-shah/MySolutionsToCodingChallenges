cmake_minimum_required(VERSION 3.14)
project(WordCountTool CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(KWordCount_Stream word_count.cpp)
add_executable(KWordCount_LocaleIndependent word_count_v2.cpp)
if (WIN32 OR MSVC)
    # Need to copy clang_rt.asan_dbg_dynamic-x86_64.dll from
    # C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\<your-version>\bin\Hostx64\x64
    #target_compile_options(KWordCount_Stream PUBLIC /fsanitize=address /W4 /WX)
    #target_compile_options(KWordCount_LocaleIndependent PUBLIC /fsanitize=address /W4 /WX)
elseif (UNIX)
    target_compile_options(KWordCount_Stream PUBLIC -fsanitize=address -Wall -Werror)
    target_link_libraries(KWordCount_Stream PRIVATE -lasan)

    target_compile_options(KWordCount_LocaleIndependent PUBLIC -fsanitize=address -Wall -Werror)
    target_link_libraries(KWordCount_LocaleIndependent PRIVATE -lasan)
endif()
