cmake_minimum_required(VERSION 3.10)
project(acc_fuzzing)

set(CMAKE_CXX_STANDARD 20)
set(SOURCE_FILES
        src/main.cpp
        src/acc_ucb.cpp
        src/context_tree.cpp
        src/js_fuzzer.cpp
)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(acc_fuzzing ${SOURCE_FILES})
