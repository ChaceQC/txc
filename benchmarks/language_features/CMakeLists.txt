cmake_minimum_required(VERSION 3.21)
project(tx_language_features_compare LANGUAGES CXX)

add_executable(language_features_cpp
    compare.cpp
    core.cpp
    containers.cpp
    objects.cpp
    memory.cpp
    workload.cpp)
target_compile_features(language_features_cpp PRIVATE cxx_std_23)
