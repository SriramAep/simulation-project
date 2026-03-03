add_executable(processor_service
    processor_service.cpp
)

target_include_directories(processor_service PRIVATE
    ${PROJECT_SOURCE_DIR}/proto
)

target_link_libraries(processor_service
    PRIVATE
    proto_lib
    observability_lib
    config_lib
    gRPC::grpc++
)