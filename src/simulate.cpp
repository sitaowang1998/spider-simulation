
#include <chrono>
#include <memory>
#include <string>
#include <variant>

#include <boost/program_options.hpp>
#include <boost/uuid.hpp>
#include <spdlog/spdlog.h>
#include <spider/core/Task.hpp>
#include <spider/storage/StorageConnection.hpp>
#include <spider/storage/mysql/MySqlStorageFactory.hpp>

#include "HeartbeatThread.hpp"
#include "JobGenerator.hpp"
#include "TaskQueue.hpp"
#include "WorkerThread.hpp"

auto parse_options(int argc, char* argv[]) -> boost::program_options::variables_map {
    boost::program_options::options_description desc;
    desc.add_options()(
        "num-jobs",
        boost::program_options::value<int>(),
        "number of jobs"
    )(
        "num-threads",
        boost::program_options::value<int>(),
        "number of worker threads"
    )(
        "num-workers",
        boost::program_options::value<int>(),
        "number of workers. Should be a multiple of num-threads"
    )(
        "storage-url",
        boost::program_options::value<std::string>(),
        "url of the mysql database"
    );

    boost::program_options::variables_map variables;
    boost::program_options::store(
        boost::program_options::parse_command_line(argc, argv, desc),
        variables
    );
    boost::program_options::notify(variables);
    return variables;
}

constexpr int cCmdArgParseError = 1;

auto main(int argc, char* argv[]) -> int {
    // Set up spdlog to write to stderr
    // NOLINTNEXTLINE(misc-include-cleaner)
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [spider.scheduler] %v");
    spdlog::set_level(spdlog::level::trace);

    boost::program_options::variables_map args = parse_options(argc, argv);

    int num_jobs, num_threads, num_workers = 0;
    std::string storage_url;
    if (args.contains("num-jobs")) {
        num_jobs = args["num-jobs"].as<int>();
    } else {
        return cCmdArgParseError;
    }
    if (args.contains("num-threads")) {
        num_threads = args["num-threads"].as<int>();
    } else {
        return cCmdArgParseError;
    }
    if (args.contains("num-workers")) {
        num_workers = args["num-workers"].as<int>();
    } else {
        return cCmdArgParseError;
    }
    if (args.contains("storage-url")) {
        storage_url = args["storage-url"].as<std::string>();
    } else {
        return cCmdArgParseError;
    }
    if (num_workers % num_threads != 0) {
        spdlog::error("Number of workers ({}) must be a multiple of number of threads ({})", num_workers, num_threads);
        return cCmdArgParseError;
    }

    std::vector<boost::uuids::uuid> worker_ids;
    worker_ids.reserve(num_workers);
    boost::uuids::random_generator uuid_gen;
    for (size_t i = 0; i < num_workers; ++i) {
        worker_ids.emplace_back(uuid_gen());
    }
    boost::uuids::uuid scheduler_id = uuid_gen();
    boost::uuids::uuid client_id = uuid_gen();

    simulation::TaskQueue task_queue{storage_url, scheduler_id};

    {
        spider::core::MySqlStorageFactory storage_factory{storage_url};
        auto conn_result = storage_factory.provide_storage_connection();
        if (std::holds_alternative<spider::core::StorageErr>(conn_result)) {
            spdlog::error("Failed to connect to the database: {}", std::get<spider::core::StorageErr>(conn_result).description);
            return cCmdArgParseError;
        }
        auto conn = std::move(std::get<std::unique_ptr<spider::core::StorageConnection>>(conn_result));
        // Initialize databases
        auto metadata_store = storage_factory.provide_metadata_storage();
        auto data_store = storage_factory.provide_data_storage();
        auto storage_error = metadata_store->initialize(*conn);
        if (!storage_error.success()) {
            spdlog::error("Failed to initialize metadata storage: {}", storage_error.description);
            return cCmdArgParseError;
        }
        storage_error = data_store->initialize(*conn);
        if (!storage_error.success()) {
            spdlog::error("Failed to initialize data storage: {}", storage_error.description);
            return cCmdArgParseError;
        }

        // Add drivers
        for (auto const& driver_id : worker_ids) {
            storage_error = metadata_store->add_driver(*conn, spider::core::Driver{driver_id});
            if (!storage_error.success()) {
                spdlog::error("Failed to add driver {}: {}", boost::uuids::to_string(driver_id), storage_error.description);
                return cCmdArgParseError;
            }
        }
        storage_error = metadata_store->add_scheduler(*conn, spider::core::Scheduler{scheduler_id, "localhost", 4343});
        if (!storage_error.success()) {
            spdlog::error("Failed to add scheduler {}: {}", boost::uuids::to_string(scheduler_id), storage_error.description);
            return cCmdArgParseError;
        }
        storage_error = metadata_store->add_driver(*conn, spider::core::Driver{client_id});
        if (!storage_error.success()) {
            spdlog::error("Failed to add client {}: {}", boost::uuids::to_string(client_id), storage_error.description);
            return cCmdArgParseError;
        }

        // Generate jobs and store them in the database
        simulation::JobGenerator job_generator{num_jobs};
        auto job_batch = storage_factory.provide_job_submission_batch(*conn);
        while (job_generator.next()) {
            auto job = job_generator.get();
            storage_error = metadata_store->add_job_batch(
                *conn,
                *job_batch,
                uuid_gen(),
                client_id,
                job
            );
            if (!storage_error.success()) {
                spdlog::error("Failed to add job to batch: {}", storage_error.description);
                return cCmdArgParseError;
            }
        }

        job_batch->submit_batch(*conn);
    }

    simulation::HeartbeatThread heartbeat_thread{storage_url, worker_ids, scheduler_id, client_id};
    simulation::Channel<size_t> task_finish_channel{};
    std::vector<simulation::WorkerThread> worker_threads;
    for (size_t i = 0; i < num_threads; ++i) {
        worker_threads.emplace_back(
            storage_url,
            num_workers / num_threads,
            task_queue,
            task_finish_channel
        );
    }

    size_t num_task_finish = 0;
    auto start = std::chrono::steady_clock::now();
    for (auto& worker_thread : worker_threads) {
        worker_thread.start();
    }
    heartbeat_thread.start();

    while (num_task_finish < num_jobs) {
        num_task_finish += task_finish_channel.receive();
    }

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    spdlog::info("Execution takes {} ms", duration.count());
    for (auto& worker_thread : worker_threads) {
        worker_thread.request_stop();
    }
    heartbeat_thread.request_stop();
    for (auto& worker_thread : worker_threads) {
        worker_thread.wait();
    }
    heartbeat_thread.wait();
}