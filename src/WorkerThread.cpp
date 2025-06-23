#include "WorkerThread.hpp"

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include <boost/uuid.hpp>
#include <spdlog/spdlog.h>
#include <spider/core/Error.hpp>
#include <spider/core/Task.hpp>
#include <spider/storage/mysql/MySqlStorageFactory.hpp>

#include "Channel.hpp"
#include "TaskQueue.hpp"

namespace simulation {

WorkerThread::WorkerThread(std::string const& storage_url, int num_workers, TaskQueue& task_queue, Channel<size_t>& channel)
    : m_storage_url{storage_url},
      m_num_workers{num_workers},
      m_task_queue{task_queue},
      m_channel{channel}
{}

auto WorkerThread::get_task(spider::core::ScheduleTaskMetadata const& task_metadata,
    spider::core::TaskInstance& task_instance) -> spider::core::StorageErr {
    auto storage_factory = spider::core::MySqlStorageFactory{m_storage_url};
    auto metadata_store = storage_factory.provide_metadata_storage();
    auto conn_result = storage_factory.provide_storage_connection();
    if (std::holds_alternative<spider::core::StorageErr>(conn_result)) {
        auto const err = std::get<spider::core::StorageErr>(conn_result);
        spdlog::error("Failed to connect to storage: {}", err.description);
        return err;
    }
    auto conn = std::move(std::get<std::unique_ptr<spider::core::StorageConnection>>(conn_result));
    task_instance.task_id = task_metadata.get_id();
    auto err = metadata_store->create_task_instance(*conn, task_instance);
    spider::core::Task task{""};
    err = metadata_store->get_task(*conn, task_metadata.get_id(), &task);
    if (!err.success()) {
        spdlog::error("Failed to get task {}: {}", to_string(task_metadata.get_id()), err.description);
        return err;
    }
    return {};
}

auto WorkerThread::submit_task(spider::core::TaskInstance const &task_instance) -> spider::core::StorageErr {
    auto storage_factory = spider::core::MySqlStorageFactory{m_storage_url};
    auto metadata_store = storage_factory.provide_metadata_storage();
    auto conn_result = storage_factory.provide_storage_connection();
    if (std::holds_alternative<spider::core::StorageErr>(conn_result)) {
        auto const err = std::get<spider::core::StorageErr>(conn_result);
        spdlog::error("Failed to connect to storage: {}", err.description);
        return err;
    }
    auto conn = std::move(std::get<std::unique_ptr<spider::core::StorageConnection>>(conn_result));
    auto err = metadata_store->task_finish(*conn, task_instance, {spider::core::TaskOutput{"0", "int"}});
    if (!err.success()) {
        spdlog::error("Failed to finish task {}: {}", to_string(task_instance.task_id), err.description);
        return err;
    }
    return {};
}

auto WorkerThread::start() -> void {
    m_thread = std::jthread([this](std::stop_token stop_token) {
        while (!stop_token.stop_requested()) {
            std::vector<spider::core::ScheduleTaskMetadata> tasks = m_task_queue.batch_pop(m_num_workers);
            if (tasks.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
            auto get_task_start_time = std::chrono::steady_clock::now();
            std::vector<spider::core::TaskInstance> task_instances;
            task_instances.reserve(tasks.size());
            for (size_t i = 0; i < tasks.size(); ++i) {
	        task_instances.emplace_back(tasks[i].get_id());
                auto const err = get_task(tasks[i], task_instances[i]);
                if (!err.success()) {
                    spdlog::error("Failed to get task {}: {}", to_string(tasks[i].get_id()), err.description);
                }
            }
            auto get_task_end_time = std::chrono::steady_clock::now();
            auto get_task_duration = std::chrono::duration_cast<std::chrono::milliseconds>(get_task_end_time - get_task_start_time);
            spdlog::info("Get task duration: {} ms", get_task_duration.count());

            std::this_thread::sleep_for(std::chrono::seconds(m_task_time));

            auto submit_task_start_time = std::chrono::steady_clock::now();
            for (auto const& instance : task_instances) {
                auto const err = submit_task(instance);
                if (!err.success()) {
                    spdlog::error("Failed to submit task {}: {}", to_string(instance.task_id), err.description);
                }
            }
            auto submit_task_end_time = std::chrono::steady_clock::now();
            auto submit_task_duration = std::chrono::duration_cast<std::chrono::milliseconds>(submit_task_end_time - submit_task_start_time);
            spdlog::info("Submit task duration: {} ms", submit_task_duration.count());

            m_channel.send(tasks.size());
        }
    });
}

auto WorkerThread::request_stop() -> void {
    m_thread.request_stop();
}


auto WorkerThread::wait() -> void {
    m_thread.join();
}


}
