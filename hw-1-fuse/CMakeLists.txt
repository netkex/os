cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_FLAGS "-Wall -g -D_FILE_OFFSET_BITS=64")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})

project(fuse-tmpfs)

include_directories(include)

find_package(Fuse REQUIRED)
include_directories(${FUSE_INCLUDE_DIR})

add_executable(vfs src/vfs.cpp src/fuse_driver.cpp src/file_system.cpp)
target_link_libraries(vfs ${FUSE_LIBRARIES})