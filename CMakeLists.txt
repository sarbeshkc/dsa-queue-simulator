cmake_minimum_required(VERSION 3.15)
project(YourProjectName)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add SDL3 path
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/libs/SDL3_install")

# Find SDL3 and SDL3_ttf
find_package(SDL3 REQUIRED)
find_package(SDL3_ttf REQUIRED)

# Source files - Fixed path for traffic_generator
set(SOURCES
    "${CMAKE_SOURCE_DIR}/src/core/main.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/window.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/text.cpp"
    "${CMAKE_SOURCE_DIR}/src/traffic/vehicle.cpp"
    "${CMAKE_SOURCE_DIR}/src/generator/traffic_generator.cpp"
)

# Header files
set(HEADERS
    "${CMAKE_SOURCE_DIR}/src/core/main.h"
    "${CMAKE_SOURCE_DIR}/src/core/window.h"
    "${CMAKE_SOURCE_DIR}/src/core/text.h"
    "${CMAKE_SOURCE_DIR}/src/traffic/vehicle.h"
    "${CMAKE_SOURCE_DIR}/src/generator/traffic_generator.h"
)

# Add executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}/src/core"
    "${CMAKE_SOURCE_DIR}/src/traffic"
    "${CMAKE_SOURCE_DIR}/src/generator"
    "${CMAKE_SOURCE_DIR}/src/config"
    "${CMAKE_SOURCE_DIR}/src/renderer"
)

# Link SDL3 and SDL3_ttf
target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
    SDL3_ttf::SDL3_ttf
)
