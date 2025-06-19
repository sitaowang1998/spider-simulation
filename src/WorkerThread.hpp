#ifndef SPIDER_SIMULATION_WORKERTHREAD_HPP
#define SPIDER_SIMULATION_WORKERTHREAD_HPP

#include <string>
#include <thread>

#include <spider/core/Error.hpp>
#include <spider/core/Task.hpp>

#include "Channel.hpp"
#include "TaskQueue.hpp"

namespace simulation {
class WorkerThread {
public:
    WorkerThread(std::string const& storage_url, int num_workers, TaskQueue& task_queue, Channel<size_t>& channel);

    auto start() -> void;
    auto request_stop() -> void;
    auto wait() -> void;
private:

    auto get_task(spider::core::ScheduleTaskMetadata const& task,
        spider::core::TaskInstance& task_instance) -> spider::core::StorageErr;

    auto submit_task(spider::core::TaskInstance const& task_instance) -> spider::core::StorageErr;

    std::string const& m_storage_url;

    int m_num_workers = 0;
    int m_task_time = 4; // Each task takes 4 seconds to complete

    TaskQueue& m_task_queue;

    Channel<size_t>& m_channel;

    std::jthread m_thread;
};
}

#endif
