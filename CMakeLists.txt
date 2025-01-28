cmake_minimum_required(VERSION 3.16)
project(comp_Assignment)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE")



add_subdirectory(libs/SDL)


# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create your executable
add_executable(comp_Assignment src/main.cpp)

target_link_libraries(comp_Assignment PRIVATE SDL3::SDL3)

# Include SDL headers
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/SDL/include
)