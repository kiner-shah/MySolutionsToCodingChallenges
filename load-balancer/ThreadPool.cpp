#include "ThreadPool.hpp"

namespace kload_balancer
{
ThreadPool::ThreadPool()
    : m_work_guard{asio::make_work_guard(m_io_context)},
    m_threads(std::thread::hardware_concurrency())
{
}

ThreadPool::~ThreadPool()
{
    m_work_guard.reset();
    for (auto& thread : m_threads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
}

void ThreadPool::start()
{
    if (m_io_context.stopped())
    {
        m_io_context.restart();
    }
    for (auto& thread : m_threads)
    {
        thread = std::thread([this]()
        {
            m_io_context.run();
        });
    }
}

void ThreadPool::stop()
{
    m_io_context.stop();
}

asio::io_context &ThreadPool::get_io_context()
{
    return m_io_context;
}

}   // namespace kload_balancer