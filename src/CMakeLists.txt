
set(SPIDER_SIMULATION_SOURCES
    simulate.cpp
    ConcurrentQueue.hpp
    JobGenerator.cpp
    JobGenerator.hpp
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
