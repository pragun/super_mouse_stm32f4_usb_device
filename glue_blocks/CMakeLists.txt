# CMakeList.txt : CMake project for TestRingBuffer, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("TestRingBuffer")

# Add source to this project's executable.
add_executable (TestRingBuffer "test_ring_buffer.cpp" "ring_buffer.cpp" "circular_buffers.cpp" )

# TODO: Add tests and install targets if needed.
