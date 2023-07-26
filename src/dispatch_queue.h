#pragma once

#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <functional>

namespace Streamer
{
    class DispatchQueue
    {
        using func_t = std::function<void (void)>;

    public:
        DispatchQueue(std::string name, std::size_t thread_count = 1);
        ~DispatchQueue();

        void dispatch(const func_t& op);
        void dispatch(func_t&& op);

        void remove_pending();

        DispatchQueue(DispatchQueue const&) = delete;
        DispatchQueue& operator=(DispatchQueue const&) = delete;
        DispatchQueue(DispatchQueue&&) = delete;
        DispatchQueue& operator=(DispatchQueue&&) = delete;

    private:
        void dispatch_thread_handler(void);

    private:
        std::string m_name;
        std::mutex m_mtx;
        std::vector<std::thread> m_threads;
        std::queue<func_t> m_queue;
        std::condition_variable m_cond_var;
        bool m_quit = false;
    };
}
