cmake_minimum_required(VERSION 3.22)
project(pie_checker)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(pie_checker main.cpp Job.cpp WorkerManager.cpp)

if(TARGET Threads::Threads)
    target_link_libraries(pie_checker PRIVATE Threads::Threads)
endif()