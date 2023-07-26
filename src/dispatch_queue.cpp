#include "dispatch_queue.h"

using namespace Streamer;

DispatchQueue::DispatchQueue(std::string name, std::size_t thread_count)
    : m_name{std::move(name)}
{
    m_threads.reserve(thread_count);
    for (std::size_t i = 0; i < thread_count; i++)
        m_threads.emplace_back(&DispatchQueue::dispatch_thread_handler, this);
}

DispatchQueue::~DispatchQueue()
{
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_quit = true;
    }
    m_cond_var.notify_all();
    for (auto& t : m_threads)
        if (t.joinable()) t.join();
}

void DispatchQueue::dispatch(const func_t& op)
{
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_queue.push(op);
    }
    m_cond_var.notify_one();
}

void DispatchQueue::dispatch(func_t&& op)
{
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_queue.push(std::move(op));
    }
    m_cond_var.notify_one();
}

void DispatchQueue::remove_pending()
{
    std::unique_lock<std::mutex> lock(m_mtx);
    m_queue = {};
}

void DispatchQueue::dispatch_thread_handler(void)
{
    std::unique_lock<std::mutex> lock(m_mtx);
    do
    {
        m_cond_var.wait(lock, [this] { return !m_queue.empty() || m_quit; });
        if (!m_quit && !m_queue.empty())
        {
            auto op = std::move(m_queue.front());
            m_queue.pop();

            lock.unlock();
            op();
            lock.lock();
        }
    } while (!m_quit);
}
