# cmake_minimum_required(VERSION 3.20)
# project(netscale LANGUAGES CXX)

# set(CMAKE_CXX_STANDARD 20)


# find_package(gRPC CONFIG REQUIRED)
# find_package(Protobuf CONFIG REQUIRED)
# find_package(spdlog CONFIG REQUIRED)
# find_package(prometheus-cpp CONFIG REQUIRED)

# Consolidated CMake Build for now
cmake_minimum_required(VERSION 3.20)

project(Simulation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(prometheus-cpp CONFIG REQUIRED)

set(PROTO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/proto/processor.proto)
get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)

set(GENERATED_SRC
    ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.cc
)

set(GENERATED_HDR
    ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.h
)

add_custom_command(
    OUTPUT ${GENERATED_SRC} ${GENERATED_HDR}
    COMMAND protobuf::protoc
    ARGS
        --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
        --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
        --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
        -I ${CMAKE_CURRENT_SOURCE_DIR}/proto
        ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
)

add_library(proto_lib ${GENERATED_SRC})
add_executable(server 
    src/main.cpp
    src/processor/processor_service.cpp
    src/observability/metrics.cpp
)

target_include_directories(proto_lib PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(proto_lib PUBLIC
    gRPC::grpc++
    protobuf::libprotobuf
)

target_link_libraries(proto_lib PUBLIC
    prometheus-cpp::core
    prometheus-cpp::pull
)

target_link_libraries(server PRIVATE proto_lib)
