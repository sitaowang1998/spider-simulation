#ifndef SPIDER_SIMULATION_HEARTBEATTHREAD_HPP
#define SPIDER_SIMULATION_HEARTBEATTHREAD_HPP

#include <string>
#include <thread>
#include <vector>

#include <boost/uuid.hpp>

namespace simulation {

class HeartbeatThread {
public:
    explicit HeartbeatThread(std::string const& storage_url, std::vector<boost::uuids::uuid> const& worker_ids,
                             boost::uuids::uuid scheduler_id, boost::uuids::uuid client_id);

    auto start() -> void;
    auto request_stop() -> void;
    auto wait() -> void;
private:
    std::string const& m_storage_url;
    std::vector<boost::uuids::uuid> const& m_worker_ids;
    boost::uuids::uuid m_scheduler_id;
    boost::uuids::uuid m_client_id;

    std::jthread m_thread;
};

}

#endif
