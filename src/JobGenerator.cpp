#include "JobGenerator.hpp"

#include <spider/core/Task.hpp>
#include <spider/core/TaskGraph.hpp>

namespace simulation {

auto JobGenerator::next() -> bool {
    if (m_current_job < m_num_jobs) {
        m_current_job++;
        return true;
    }
    return false;
}

auto JobGenerator::get() -> spider::core::TaskGraph {
    spider::core::Task task{"task"};
    auto task_input = spider::core::TaskInput{"0", "int"};
    task.add_input(task_input);
    task.add_output(spider::core::TaskOutput{"int"});
    spider::core::TaskGraph graph;
    graph.add_task(task);
    graph.add_input_task(task.get_id());
    graph.add_output_task(task.get_id());
    return graph;
}


}
