add_executable(cppesphome main.cpp)
target_link_libraries(cppesphome PUBLIC 
    Threads::Threads
    protobuf::libprotobuf
    Boost::headers
    STDEXEC::stdexec
)
# workaround for link errors...  https://github.com/grpc/grpc/issues/34046 
if (Protobuf_VERSION VERSION_GREATER_EQUAL 4)
    find_package(absl REQUIRED)
    target_link_libraries (cppesphome
        PUBLIC
            absl::log_internal_check_op
    )
endif()

set(proto_inc_dir "${CMAKE_CURRENT_LIST_DIR}")
protobuf_generate_cpp(
    PROTO_SRCS PROTO_HDRS DESCRIPTORS PROTO_DESCS
    "${proto_inc_dir}/api.proto"
    "${proto_inc_dir}/api_options.proto"
)
target_sources(cppesphome PRIVATE ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(cppesphome PRIVATE  "${CMAKE_CURRENT_BINARY_DIR}")
