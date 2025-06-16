
#include <boost/program_options.hpp>
#include <spdlog/spdlog.h>

#include "JobGenerator.hpp"

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

    simulation::JobGenerator job_generator{num_jobs};
}