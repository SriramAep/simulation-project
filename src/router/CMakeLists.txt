add_executable(router_service
    router_service.cpp
)

target_include_directories(router_service PRIVATE
    ${PROJECT_SOURCE_DIR}/proto
)

target_link_libraries(router_service
    PRIVATE
    proto_lib
    observability_lib
    config_lib
    gRPC::grpc++
)