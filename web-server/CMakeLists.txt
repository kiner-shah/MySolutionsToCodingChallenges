cmake_minimum_required(VERSION 3.14)
project(WebServer CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(KWebServer 
    Client.cpp
    ClientManager.cpp
    HttpStatusCodes.cpp
    HttpRequestParser.cpp
    HttpRequest.cpp
    HttpResponse.cpp
    ThreadPool.cpp
    WebServer.cpp
    main.cpp)
if (WIN32 OR MSVC)
    # Need to copy clang_rt.asan_dbg_dynamic-x86_64.dll from
    # C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\<your-version>\bin\Hostx64\x64
    #target_compile_options(KWebServer PUBLIC /fsanitize=address /W4 /WX)
elseif (UNIX)
    # target_compile_options(KWebServer PUBLIC -fsanitize=address -Wall -Werror)
    # target_link_libraries(KWebServer PRIVATE -lasan)
endif()

target_include_directories(KWebServer PRIVATE asio-1.30.2/include)

find_package(spdlog 1.15.2 REQUIRED HINTS spdlog-1.15.2-install)
target_link_libraries(KWebServer PRIVATE spdlog::spdlog)