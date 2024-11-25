cmake_minimum_required(VERSION 3.10)

project(vshttp VERSION 0.0.2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Build a fat binary for arm64/x86_64
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
endif()

# Collect source files
file(GLOB SOURCES "src/*.cpp")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "build")



# Add executable
add_executable(vshttp ${SOURCES})

# Set the compiler
set(CMAKE_CXX_COMPILER "clang++")

# Run the executable from the build directory
add_custom_target(run
    COMMAND make .. && ./vshttp
    DEPENDS vshttp
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Building and running vshttp from the build directory"
)


# Run formatter
add_custom_target(format
    COMMAND find src -name '*.hpp' -o -name '*.cpp' | xargs clang-format -i
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Formatting source files"
)
