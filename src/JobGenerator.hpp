#ifndef SPIDER_SIMULATION_JOBGENERATOR_HPP
#define SPIDER_SIMULATION_JOBGENERATOR_HPP

#include <spider/core/TaskGraph.hpp>

namespace simulation {

class JobGenerator {
public:
    explicit JobGenerator(long const num_jobs) : m_num_jobs(num_jobs) {}

    auto next() -> bool;
    auto get() -> spider::core::TaskGraph;

private:
    long m_num_jobs;
    long m_current_job = 0;
};

}

#endif
