add_library(cppesphomeapi)
generate_export_header(cppesphomeapi EXPORT_FILE_NAME cppesphomeapi/cppesphomeapi_export.hpp)
target_include_directories(cppesphomeapi
    PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

set(public_inc_dir include/cppesphomeapi)
target_sources(cppesphomeapi
    PUBLIC
        FILE_SET headers
            TYPE HEADERS
            BASE_DIRS include
            FILES
                ${public_inc_dir}/api_client.hpp
                ${public_inc_dir}/async_result.hpp
                ${public_inc_dir}/result.hpp
                ${public_inc_dir}/detail/awaitable.hpp
        FILE_SET generated_headers
            TYPE HEADERS
            BASE_DIRS ${CMAKE_CURRENT_BINARY_DIR}
            FILES
                ${CMAKE_CURRENT_BINARY_DIR}/cppesphomeapi/cppesphomeapi_export.hpp
                ${CMAKE_CURRENT_BINARY_DIR}/cppesphomeapi/version.hpp
)


target_link_libraries(cppesphomeapi PUBLIC
    Threads::Threads
    protobuf::libprotobuf
    Boost::headers
)

## PROTOBUF START ##
# workaround for link errors...  https://github.com/grpc/grpc/issues/34046
if (Protobuf_VERSION VERSION_GREATER_EQUAL 4)
    find_package(absl REQUIRED)
    target_link_libraries (cppesphomeapi
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
target_sources(cppesphomeapi PRIVATE ${PROTO_SRCS} ${PROTO_HDRS})
## PROTOBUF END ##

# configure version file
configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/version.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cppesphomeapi/version.hpp"
    @ONLY
)

add_subdirectory(src)
