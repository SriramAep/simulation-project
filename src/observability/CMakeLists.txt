add_library(observability_lib
    logging/logger.cpp
    metrics/metrics.cpp
)

target_include_directories(observability_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(observability_lib
    PUBLIC
    prometheus-cpp::core
    prometheus-cpp::pull
    spdlog::spdlog
)