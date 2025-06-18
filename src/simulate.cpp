
#include <string>
#include <variant>

#include <boost/program_options.hpp>
#include <boost/uuid.hpp>
#include <spdlog/spdlog.h>
#include <spider/storage/mysql/MySqlStorageFactory.hpp>

#include "JobGenerator.hpp"
#include "spider/storage/mysql/MySqlConnection.hpp"

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
#ifndef NDEBUG
    spdlog::set_level(spdlog::level::trace);
#endif

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

    {
        spider::core::MySqlStorageFactory storage_factory{storage_url};
        auto conn_result = storage_factory.provide_storage_connection();
        if (std::holds_alternative<spider::core::StorageErr>(conn_result)) {
            spdlog::error("Failed to connect to the database: {}", std::get<spider::core::StorageErr>(conn_result).description);
            return cCmdArgParseError;
        }
        auto conn = std::move(std::get<spider::core::MySqlConnection>(conn_result));
        // Initialize databases
        auto metadata_store = storage_factory.provide_metadata_storage();
        auto data_store = storage_factory.provide_data_storage();
        auto storage_error = metadata_store->initialize(conn);
        if (!storage_error.success()) {
            spdlog::error("Failed to initialize metadata storage: {}", storage_error.description);
            return cCmdArgParseError;
        }
        storage_error = data_store->initialize(conn);
        if (!storage_error.success()) {
            spdlog::error("Failed to initialize data storage: {}", storage_error.description);
            return cCmdArgParseError;
        }

        // Generate jobs and store them in the database
        simulation::JobGenerator job_generator{num_jobs};
        boost::uuids::random_generator uuid_gen;
        auto client_id = uuid_gen();
        auto job_batch = storage_factory.provide_job_submission_batch(conn);
        while (job_generator.next()) {
            auto job = job_generator.get();
            storage_error = metadata_store->add_job_batch(
                conn,
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

        job_batch->submit_batch(conn);
    }


}