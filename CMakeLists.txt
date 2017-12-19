cmake_minimum_required(VERSION 2.6)
project(HTTPinger)
set(CMAKE_CXX_FLAGS "-std=c++1y -Wall -lrt")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wunused-result")
message("CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")

add_executable(http_pinger src/Main.cpp)
target_link_libraries(http_pinger -lrt)

set_target_properties(http_pinger PROPERTIES DEBUG_POSTFIX "_d")
set_target_properties(http_pinger PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
