cmake_minimum_required(VERSION 3.14)
project(os_ex4)

set(CMAKE_CXX_STANDARD 14)

add_executable(os_ex4 SimpleTest.cpp PhysicalMemory.h MemoryConstants.h VirtualMemory.h PhysicalMemory.cpp VirtualMemory.cpp)