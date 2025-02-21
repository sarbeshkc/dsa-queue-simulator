cmake_minimum_required(VERSION 3.10)
project(TrafficSimulator VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add SDL3 installation path
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/libs/SDL3_install")

# Find SDL3
find_package(SDL3 REQUIRED)

# Add include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/traffic_generator/include
)

# Define shared sources
set(SHARED_SOURCES
    src/core/Vehicle.cpp
)

# Define simulator sources
set(SIMULATOR_SOURCES
    src/main.cpp
    src/core/Lane.cpp
    src/core/TrafficLight.cpp
    src/managers/TrafficManager.cpp
    src/managers/IntersectionController.cpp
    src/managers/FileHandler.cpp
    src/utils/Queue.cpp
    src/visualization/Renderer.cpp
    src/visualization/DebugOverlay.cpp
    ${SHARED_SOURCES}
)

# Define generator sources
set(GENERATOR_SOURCES
    traffic_generator/src/main.cpp
    traffic_generator/src/Generator.cpp
    ${SHARED_SOURCES}
)

# Main simulator executable
add_executable(simulator ${SIMULATOR_SOURCES})

# Traffic generator executable
add_executable(traffic_generator ${GENERATOR_SOURCES})

# Link SDL3 to simulator
target_link_libraries(simulator PRIVATE SDL3::SDL3)

# Create data directory in build directory
add_custom_command(
    TARGET simulator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    ${CMAKE_BINARY_DIR}/bin/data/lanes
)

add_custom_command(
    TARGET traffic_generator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    ${CMAKE_BINARY_DIR}/bin/data/lanes
)

# Optional: Add compiler warnings
if(MSVC)
    target_compile_options(simulator PRIVATE /W4)
    target_compile_options(traffic_generator PRIVATE /W4)
else()
    target_compile_options(simulator PRIVATE -Wall -Wextra)
    target_compile_options(traffic_generator PRIVATE -Wall -Wextra)
endif()

# Print configuration information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "SDL3 found: ${SDL3_FOUND}")
message(STATUS "SDL3 version: ${SDL3_VERSION}")
message(STATUS "Binary directory: ${CMAKE_BINARY_DIR}/bin")
message(STATUS "Data directory: ${CMAKE_BINARY_DIR}/bin/data/lanes")
