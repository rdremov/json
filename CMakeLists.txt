# CMakeList.txt : CMake project for rvdjson, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("json")

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add source to this project's executable.
add_executable (json "json.h" "node.h" "test.cpp")

# TODO: Add tests and install targets if needed.
