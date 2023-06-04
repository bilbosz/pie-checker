cmake_minimum_required(VERSION 3.22)
project(pie_checker)

set(CMAKE_CXX_STANDARD 20)

add_executable(pie_checker main.cpp Worker.cpp Job.cpp WorkerManager.cpp)