cmake_minimum_required(VERSION 3.20)
project(qpp_bench CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)
FetchContent_Declare(
  qpp
  GIT_REPOSITORY https://github.com/softwareQinc/qpp.git
  GIT_TAG v7.0.3)
FetchContent_MakeAvailable(qpp)

add_executable(qpp_bench qpp_bench.cpp)
# `libqpp_internal` is qpp's build-tree target: the qpp headers plus Eigen,
# threads, and its compiler flags.
target_link_libraries(qpp_bench PUBLIC libqpp_internal)
