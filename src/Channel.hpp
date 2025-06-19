#ifndef SPIDER_SIMULATION_CHANNEL_HPP
#define SPIDER_SIMULATION_CHANNEL_HPP

#include <deque>
#include <condition_variable>
#include <mutex>

namespace simulation {
template<typename T>
class Channel {
public:

    auto push(T const& t) -> void {
        {
            std::lock_guard lock{m_mutex};
            m_queue.push_back(t);
        }
        m_cond_var.notify_all();
    }

    auto pop() -> T {
        std::lock_guard lock {m_mutex};
        m_cond_var.wait(lock, [this]{ return !m_queue.empty(); });
        T t = std::move(m_queue.front());
        m_queue.pop_front();
        return t;
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cond_var;

    std::deque<T> m_queue;
};
}

#endif
