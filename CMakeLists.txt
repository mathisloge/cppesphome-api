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
include(dependencies)

declare_dependencies()

find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(stdexec CONFIG REQUIRED)

# only for asio. TODO: make using standalone asio possible
find_package(Boost CONFIG REQUIRED)

add_subdirectory(cppesphomeapi)
