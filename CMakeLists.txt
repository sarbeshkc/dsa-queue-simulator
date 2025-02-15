# CMakeLists.txt

cmake_minimum_required(VERSION 3.15)
project(TrafficSimulation)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/libs/SDL3_install")
find_package(SDL3 REQUIRED)

# Updated SOURCES list without lane_processor.cpp
set(SOURCES
    src/core/main.cpp
    src/core/window.cpp
    src/traffic/lane.cpp
    src/traffic/traffic_light.cpp
    src/traffic/vehicle.cpp
    src/traffic/traffic_manager.cpp
    src/traffic/road_system.cpp
    src/traffic/traffic_coordinator.cpp
    src/communication/file_handler.cpp
    src/traffic/traffic_communicator.cpp
    src/generator/traffic_generator.cpp
)

# Headers list remains the same because we do have lane_processor.h
set(HEADERS
    src/core/main.h
    src/core/window.h
    src/core/queue.h

    src/traffic/lane.h
    src/traffic/traffic_light.h
    src/traffic/vehicle.h
    src/traffic/traffic_manager.h
    src/traffic/road_system.h
    src/traffic/traffic_coordinator.h
    src/traffic/lane_processor.h

    src/communication/file_handler.h
    src/traffic/traffic_communicator.h

    src/generator/traffic_generator.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/traffic
    ${CMAKE_SOURCE_DIR}/src/communication
    ${CMAKE_SOURCE_DIR}/src/generator

    ${CMAKE_SOURCE_DIR}/src/common    # Add this line
)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)

if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/traffic_data
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()
