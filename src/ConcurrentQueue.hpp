#ifndef SPIDER_SIMULATION_CONCURRENTQUEUE_CPP
#define SPIDER_SIMULATION_CONCURRENTQUEUE_CPP

#include <deque>
#include <mutex>
#include <optional>
#include <vector>

namespace simulation {

template<typename T>
class ConcurrentQueue<T> {
public:
    auto push(T value) -> void {
        std::lock_guard<std::mutex> lock{m_mutex};
        m_queue.push(std::move(value));
    }

    auto pop() -> std::optional<T> {
        std::lock_guard<std::mutex> lock{m_mutex};
        if (m_queue.empty()) {
            return std::nullopt;
        }
        T value = std::move(m_queue.front());
        m_queue.pop_front();
        return value;
    }

    auto batch_push(std::vector<T> const& value) -> void {
        std::lock_guard<std::mutex> lock{m_mutex};
        for (const auto& item : value) {
            m_queue.push_back(item);
        }
    }

    auto batch_pop(size_t count) -> std::vector<T> {
        std::lock_guard<std::mutex> lock{m_mutex};
        std::vector<T> result;
        result.reserve(count);
        for (size_t i = 0; i < count && !m_queue.empty(); ++i) {
            result.push_back(std::move(m_queue.front()));
            m_queue.pop_front();
        }
        return result;
    }

private:
    std::mutex m_mutex;
    std::deque<T> m_queue;
};

}

#endif
