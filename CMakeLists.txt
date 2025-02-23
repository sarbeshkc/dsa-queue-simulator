cmake_minimum_required(VERSION 3.15)
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

# Define include directories with proper scope
include_directories(
    ${PROJECT_SOURCE_DIR}/include
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
    src/managers/FileHandler.cpp
    src/managers/IntersectionController.cpp
    src/managers/TrafficManager.cpp
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

# Add executables
add_executable(simulator ${SIMULATOR_SOURCES})
add_executable(traffic_generator ${GENERATOR_SOURCES})

# Set include directories for each target
target_include_directories(simulator PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_include_directories(traffic_generator PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/traffic_generator/include
)

# Link SDL3 to simulator
target_link_libraries(simulator PRIVATE SDL3::SDL3)

# Set compile definitions for MSVC
if(MSVC)
    target_compile_definitions(simulator PRIVATE
        _USE_MATH_DEFINES
        _CRT_SECURE_NO_WARNINGS
    )
    target_compile_definitions(traffic_generator PRIVATE
        _USE_MATH_DEFINES
        _CRT_SECURE_NO_WARNINGS
    )
endif()

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

if(MSVC)
    add_compile_definitions(
        _USE_MATH_DEFINES
        _CRT_SECURE_NO_WARNINGS
    )
    # Disable specific warnings
    add_compile_options(
        /wd4100  # Unreferenced formal parameter
        /wd4189  # Local variable is initialized but not referenced
        /wd4244  # Conversion from 'double' to 'float', possible loss of data
        /wd4305  # Truncation from 'double' to 'float'
    )
endif()
