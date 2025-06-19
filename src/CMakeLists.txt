
set(SPIDER_SIMULATION_SOURCES
    simulate.cpp
    Channel.hpp
    HeartbeatThread.cpp
    HeartbeatThread.hpp
    JobGenerator.cpp
    JobGenerator.hpp
    TaskQueue.hpp
    TaskQueue.cpp
    WorkerThread.cpp
    WorkerThread.hpp
)

add_executable(spider-simulation)
target_sources(spider-simulation PRIVATE ${SPIDER_SIMULATION_SOURCES})
target_link_libraries(spider-simulation
    PRIVATE
    spider_core
    spider_client
    Boost::headers
    Boost::program_options
    spdlog::spdlog
)
