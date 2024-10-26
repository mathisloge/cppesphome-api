add_library(cppesphome-api)
target_link_libraries(cppesphome-api PUBLIC asio-grpc::asio-grpc-stdexec STDEXEC::stdexec)


set(proto_inc_dir "${CMAKE_CURRENT_LIST_DIR}")
asio_grpc_protobuf_generate(
    GENERATE_GRPC
    OUT_VAR proto_srcs
    OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated"
    IMPORT_DIRS "${CMAKE_CURRENT_LIST_DIR}"
    PROTOS
        "${proto_inc_dir}/api.proto"
        "${proto_inc_dir}/api_options.proto"
)
target_sources(cppesphome-api PRIVATE ${proto_srcs})
