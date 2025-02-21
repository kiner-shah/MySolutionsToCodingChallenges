cmake_minimum_required(VERSION 3.14)
project(JsonParserTool CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(KJsonParser Tokenizer.cpp Parser.cpp json_parser.cpp)
target_compile_options(KJsonParser PUBLIC -fsanitize=address -Wall -Werror)
target_link_libraries(KJsonParser PRIVATE -lasan)