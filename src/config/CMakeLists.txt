add_library(config_lib
    config.cpp
)

target_include_directories(config_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(config_lib 
    PRIVATE
    yaml-cpp::yaml-cpp
    spdlog::spdlog
)