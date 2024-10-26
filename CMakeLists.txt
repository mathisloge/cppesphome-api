cmake_minimum_required(VERSION 3.30)
project(cppesphome-api VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/runtime")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CTest)
include(FetchContent)
include(GNUInstallDirs)
include(GenerateExportHeader)
include(dependencies)

declare_dependencies()

find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)

# only for asio. TODO: make using standalone asio possible
find_package(Boost CONFIG REQUIRED)

add_subdirectory(cppesphomeapi)
add_subdirectory(example)

include(CMakePackageConfigHelpers)

install(TARGETS cppesphomeapi
    EXPORT cppesphomeapiTargets
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILE_SET headers
    FILE_SET generated_headers
)

set(config_install_dir "${CMAKE_INSTALL_LIBDIR}/cmake/cppesphomeapi")

install(EXPORT cppesphomeapiTargets
    FILE cppesphomeapiTargets.cmake
    NAMESPACE quite::
    DESTINATION "${config_install_dir}"
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cppesphomeapiConfig.cmake"
    INSTALL_DESTINATION "${config_install_dir}"
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/cppesphomeapiConfigVersion.cmake"
  VERSION "${CMAKE_PROJECT_VERSION}"
  COMPATIBILITY SameMajorVersion
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/cppesphomeapiConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/cppesphomeapiConfigVersion.cmake"
  DESTINATION "${config_install_dir}"
)
