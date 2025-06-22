#include "HeartbeatThread.hpp"

#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include <boost/uuid.hpp>
#include <spdlog/spdlog.h>
#include <spider/core/Error.hpp>
#include <spider/storage/StorageConnection.hpp>
#include <spider/storage/mysql/MySqlStorageFactory.hpp>

namespace simulation {

HeartbeatThread::HeartbeatThread(std::string const& storage_url, std::vector<boost::uuids::uuid> const& worker_ids,
    boost::uuids::uuid const scheduler_id, boost::uuids::uuid const client_id)
    : m_storage_url{storage_url},
      m_worker_ids{worker_ids},
      m_scheduler_id{scheduler_id},
      m_client_id{client_id}
{}

auto send_heartbeat(std::string const& storage_url, boost::uuids::uuid const& driver_id) -> spider::core::StorageErr {
    auto storage_factory = spider::core::MySqlStorageFactory{storage_url};
    auto metadata_store = storage_factory.provide_metadata_storage();
    auto conn_result = storage_factory.provide_storage_connection();
    if (std::holds_alternative<spider::core::StorageErr>(conn_result)) {
        return std::get<spider::core::StorageErr>(conn_result);
    }
    auto conn = std::move(std::get<spider::core::StorageConnection>(conn_result));
    return metadata_store->update_heartbeat(conn, driver_id);
}

auto HeartbeatThread::start() -> void {
    m_thread = std::jthread([this](std::stop_token stop_token) {
        while (!stop_token.stop_requested()) {
            auto start = std::chrono::system_clock::now();
            for (boost::uuids::uuid const& worker_id : m_worker_ids) {
                auto err = send_heartbeat(m_storage_url, worker_id);
                if (!err.success()) {
                    spdlog::error("Cannot update heartbeat for worker {}: {}", to_string(worker_id), err.description);
                }
            }
            auto err = send_heartbeat(m_storage_url, m_scheduler_id);
            if (!err.success()) {
                spdlog::error("Cannot update heartbeat for scheduler {}: {}", to_string(m_scheduler_id), err.description);
            }
            err = send_heartbeat(m_storage_url, m_client_id);
            if (!err.success()) {
                spdlog::error("Cannot update heartbeat for client {}: {}", to_string(m_client_id), err.description);
            }
            auto end = std::chrono::system_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            spdlog::info("Heartbeat sent, took {} milliseconds", duration.count());
            auto sleep_duration = std::chrono::milliseconds(1000) - duration;
            if (sleep_duration > std::chrono::milliseconds(0)) {
                std::this_thread::sleep_for(sleep_duration);
            }
        }
    });
}

auto HeartbeatThread::request_stop() -> void {
    m_thread.request_stop();
}

auto HeartbeatThread::wait() -> void {
    m_thread.join();
}


}
