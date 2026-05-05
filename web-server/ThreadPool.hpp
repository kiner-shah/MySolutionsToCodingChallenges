#pragma once

#include <asio/io_context.hpp>
#include <vector>
#include <thread>

namespace kweb_server
{
class ThreadPool
{
    asio::io_context m_io_context;
    asio::executor_work_guard<asio::io_context::executor_type> m_work_guard;
    std::vector<std::thread> m_threads;
public:
    ThreadPool();
    ~ThreadPool();
    void start();
    void stop();
    asio::io_context& get_io_context();
};
}   // namespace kweb_server