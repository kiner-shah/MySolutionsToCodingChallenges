cmake_minimum_required(VERSION 3.14)
project(WordCountTool CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(KWordCount_Stream word_count.cpp)
target_compile_options(KWordCount_Stream PUBLIC -fsanitize=address -Wall -Werror)
target_link_libraries(KWordCount_Stream PRIVATE -lasan)