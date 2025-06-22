#include "TaskQueue.hpp"

#include <chrono>
#include <string>
#include <variant>
#include <vector>

#include <boost/uuid.hpp>
#include <spdlog/spdlog.h>
#include <spider/core/Error.hpp>
#include <spider/storage/mysql/MySqlStorageFactory.hpp>

namespace simulation {

TaskQueue::TaskQueue(std::string const& storage_url, boost::uuids::uuid const scheduler_id)
    : m_scheduler_id{scheduler_id} {
    auto storage_factory = spider::core::MySqlStorageFactory{storage_url};
    auto conn_result = storage_factory.provide_storage_connection();
    if (std::holds_alternative<spider::core::StorageErr>(conn_result)) {
        spdlog::error("Failed to connect to storage: {}",
                      std::get<spider::core::StorageErr>(conn_result).description);
        throw std::runtime_error("Failed to connect to storage");
    }
    m_conn = std::move(std::get<spider::core::StorageConnection>(conn_result));
    m_store = storage_factory.provide_metadata_storage();
}

auto TaskQueue::batch_pop(int count) -> std::vector<spider::core::ScheduleTaskMetadata> {
    std::lock_guard lock{m_mutex};
    if (m_task_queue.empty()) {
        // Fetch tasks from the storage if the queue is empty
        std::vector<spider::core::ScheduleTaskMetadata> tasks;
        auto const start_time = std::chrono::steady_clock::now();
        auto err = m_store->get_ready_tasks(m_conn, m_scheduler_id, &tasks);
        auto const end_time = std::chrono::steady_clock::now();
        auto const duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        spdlog::info("Fetched {} tasks from storage in {} ms", tasks.size(), duration.count());
        if (!err.success()) {
            spdlog::error("Failed to fetch tasks from storage: {}",
                          err.description);
            return {};
        }
        if (tasks.empty()) {
            return {};
        }
        for (auto& task : tasks) {
            m_task_queue.push_back(std::move(task));
        }
    }
    std::vector<spider::core::ScheduleTaskMetadata> result;
    result.reserve(count);
    for (size_t i = 0; i < count && !m_task_queue.empty(); ++i) {
        result.emplace_back(std::move(m_task_queue.front()));
        m_task_queue.pop_front();
    }
    return result;
}


}
