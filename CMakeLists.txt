cmake_minimum_required(VERSION 3.10)

project(SoftwareEngProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(WUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(ext/wui)

add_executable(hello_world src/main.cpp)

target_link_libraries(hello_world PRIVATE wui)