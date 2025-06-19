#ifndef SPIDER_SIMULATION_SCHEDULERTHREAD_HPP
#define SPIDER_SIMULATION_SCHEDULERTHREAD_HPP

#include <memory>
#include <string>
#include <thread>

#include <boost/uuid.hpp>
#include <spider/core/Task.hpp>
#include <spider/storage/StorageConnection.hpp>
#include <spider/storage/MetadataStorage.hpp>

#include "ConcurrentQueue.hpp"

namespace simulation {

class SchedulerThread {
public:
    SchedulerThread(std::string const& storage_url, boost::uuids::uuid m_scheduler_id,
        ConcurrentQueue<spider::core::ScheduleTaskMetadata>& task_queue);

    auto start() -> void;

    auto request_stop() -> void;
    auto wait() -> void;

private:
    std::unique_ptr<spider::core::MetadataStorage> m_store;
    spider::core::StorageConnection m_conn;

    boost::uuids::uuid m_scheduler_id;

    ConcurrentQueue<spider::core::ScheduleTaskMetadata>& m_task_queue;

    std::jthread m_thread;
};

}

#endif
