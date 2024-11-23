cmake_minimum_required(VERSION 3.10)

project(vshttp VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set architecture for macOS
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()

# Collect source files
file(GLOB SOURCES "src/*.cpp")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "build")

# Add executable
add_executable(vshttp ${SOURCES})

# Optionally set the compiler
set(CMAKE_CXX_COMPILER "clang++")

# Run the executable from the build directory
add_custom_target(run
    COMMAND ./vshttp
    DEPENDS vshttp
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Running vshttp from the build directory"
)