cmake_minimum_required(VERSION 3.12)
project(cxx_calc)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Automatically collect all .cpp and .hpp files in the src/ directory
# CONFIGURE_DEPENDS tells CMake to automatically re-run and detect new files when you build
file(GLOB SRC_FILES CONFIGURE_DEPENDS "src/*.cpp" "src/*.hpp")

add_executable(calc ${SRC_FILES})
