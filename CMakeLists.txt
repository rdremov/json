# CMakeList.txt : CMake project for rvdjson, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("json")

# Add source to this project's executable.
add_executable (json "json.h" "test.cpp")

# TODO: Add tests and install targets if needed.
