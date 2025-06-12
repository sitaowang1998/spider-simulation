
set(SPIDER_SIMULATION_SOURCES
    simulate.cpp
)

add_executable(spider-simulation)
target_sources(spider-simulation PRIVATE ${SPIDER_SIMULATION_SOURCES})
target_link_libraries(spider-simulation
    PRIVATE
    spider_core
)