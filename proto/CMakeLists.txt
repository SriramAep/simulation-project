set(PROTO_FILES
    common.proto
    gateway.proto
    processor.proto
    router.proto
)

set(GENERATED_SRCS)
set(GENERATED_HDRS)

foreach(PROTO ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO} NAME_WE)

    list(APPEND GENERATED_SRCS
        ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.cc
    )

    list(APPEND GENERATED_HDRS
        ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.h
        ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.h
    )

    add_custom_command(
        OUTPUT
            ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.cc
            ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.cc
            ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.h
            ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.h
        COMMAND protobuf::protoc
        ARGS
            --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
            --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
            --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
            -I ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/${PROTO}
        DEPENDS ${PROTO}
    )
endforeach()

add_library(proto_lib ${GENERATED_SRCS})

target_include_directories(proto_lib PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(proto_lib PUBLIC
    gRPC::grpc++
    protobuf::libprotobuf
)