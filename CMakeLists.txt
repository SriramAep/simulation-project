# Consolidated CMake Build for now
cmake_minimum_required(VERSION 3.20)
project(Simulation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(prometheus-cpp CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

add_subdirectory(proto)
add_subdirectory(src/config)
add_subdirectory(src/observability)
add_subdirectory(src/processor)
add_subdirectory(src/router)
add_subdirectory(src/gateway)