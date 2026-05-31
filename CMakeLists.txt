cmake_minimum_required(VERSION 3.16)
project(hdr_scanner VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    $<$<CONFIG:Release>:-O2>          # autovectorization + optimizations
    $<$<CONFIG:Release>:-march=native> # use all CPU features available
    $<$<CONFIG:Debug>:-g>
)


add_executable(hdr_scanner src/main.cpp)

target_include_directories(hdr_scanner PUBLIC include)

install(TARGETS hdr_scanner DESTINATION bin)
find_package(pybind11 QUIET)
if(pybind11_FOUND)
    pybind11_add_module(hdr_py src/hdr_py.cpp)
    target_include_directories(hdr_py PRIVATE include)
    target_link_libraries(hdr_py PRIVATE pybind11::headers)
    set_target_properties(hdr_py PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
else()
    message(STATUS "pybind11 not found — skipping Python bindings")
endif()
