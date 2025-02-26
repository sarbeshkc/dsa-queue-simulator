cmake_minimum_required(VERSION 3.15)
project(TrafficJunctionSimulator VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Find SDL3
find_package(SDL3 REQUIRED)

# Create include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Add source files
set(SIMULATOR_SOURCES
    src/main.cpp
    src/utils/DebugLogger.cpp
)

set(GENERATOR_SOURCES
    src/traffic_generator.cpp
)

# Create executables
add_executable(simulator ${SIMULATOR_SOURCES})
add_executable(traffic_generator ${GENERATOR_SOURCES})

# Link SDL3 to simulator
target_link_libraries(simulator PRIVATE SDL3::SDL3)

# Handle platform-specific settings
if(MSVC)
    # MSVC-specific compiler settings
    target_compile_definitions(simulator PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(traffic_generator PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
else()
    # GCC/Clang settings
    target_compile_options(simulator PRIVATE -Wall -Wextra)
    target_compile_options(traffic_generator PRIVATE -Wall -Wextra)
endif()

# Create data directory in build folder
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

# Print configuration summary
message(STATUS "Build configuration:")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
