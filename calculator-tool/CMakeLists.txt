cmake_minimum_required(VERSION 3.14)
project(CalculatorTool CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(KCalculator calculator.cpp)
if (WIN32 OR MSVC)
    # Need to copy clang_rt.asan_dbg_dynamic-x86_64.dll from
    # C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\<your-version>\bin\Hostx64\x64
    #target_compile_options(KCalculator PUBLIC /fsanitize=address /W4 /WX)
elseif (UNIX)
    target_compile_options(KCalculator PUBLIC -fsanitize=address -Wall -Werror -Wswitch-default -Wswitch-enum)
    target_link_libraries(KCalculator PRIVATE -lasan)
endif()
