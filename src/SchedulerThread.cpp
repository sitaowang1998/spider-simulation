#include "SchedulerThread.hpp"

#include <chrono>
#include <string>
#include <variant>

#include <boost/uuid.hpp>
#include <spdlog/spdlog.h>
#include <spider/core/Error.hpp>
#include <spider/core/Task.hpp>
#include <spider/storage/mysql/MySqlStorageFactory.hpp>


namespace simulation {

SchedulerThread::SchedulerThread(std::string const &storage_url, boost::uuids::uuid const scheduler_id
    , ConcurrentQueue<spider::core::ScheduleTaskMetadata> &task_queue)
    : m_scheduler_id{scheduler_id},
      m_task_queue{task_queue}
{
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

auto SchedulerThread::start() -> void {
    m_thread = std::jthread{[this](std::stop_token stop_token) {
        while (!stop_token.stop_requested()) {
            if (!m_task_queue.empty()) {
                continue;
            }
            std::vector<spider::core::ScheduleTaskMetadata> tasks;
            m_store->get_ready_tasks(m_conn, m_scheduler_id, &tasks);
            m_task_queue.batch_push(tasks);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }};
}


auto SchedulerThread::request_stop() -> void {
    m_thread.request_stop();
}

auto SchedulerThread::wait() -> void {
    m_thread.join();
}

}
