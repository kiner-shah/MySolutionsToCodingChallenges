cmake_minimum_required(VERSION 3.14)
project(LoadBalancer CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(KLoadBalancer load_balancer.cpp LoadBalancer.cpp UserClient.cpp Client.cpp)
if (WIN32 OR MSVC)
    # Need to copy clang_rt.asan_dbg_dynamic-x86_64.dll from
    # C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\<your-version>\bin\Hostx64\x64
    #target_compile_options(KLoadBalancer PUBLIC /fsanitize=address /W4 /WX)
elseif (UNIX)
    target_compile_options(KLoadBalancer PUBLIC -fsanitize=thread -Wall -Werror)
    target_link_libraries(KLoadBalancer PRIVATE -ltsan)
endif()
target_include_directories(KLoadBalancer PRIVATE asio-1.30.2/include)

find_package(spdlog 1.15.2 REQUIRED HINTS spdlog-1.15.2-install)
target_link_libraries(KLoadBalancer PRIVATE spdlog::spdlog)
