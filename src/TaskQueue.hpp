#ifndef SPIDER_SIMULATION_TASKQUEUE_HPP
#define SPIDER_SIMULATION_TASKQUEUE_HPP

#include <deque>
#include <mutex>
#include <string>
#include <vector>

#include <boost/uuid.hpp>
#include <spider/core/Task.hpp>
#include <spider/storage/MetadataStorage.hpp>
#include <spider/storage/StorageConnection.hpp>

namespace simulation {
class TaskQueue {
public:
    TaskQueue(std::string const& storage_url, boost::uuids::uuid scheduler_id);

    auto batch_pop(int count) -> std::vector<spider::core::ScheduleTaskMetadata>;
private:
    std::unique_ptr<spider::core::MetadataStorage> m_store;
    std::unique_ptr<spider::core::StorageConnection> m_conn;

    boost::uuids::uuid m_scheduler_id;

    std::mutex m_mutex;
    std::deque<spider::core::ScheduleTaskMetadata> m_task_queue;
};
}

#endif
