cmake_minimum_required(VERSION 3.21)
project(tx_library_compare LANGUAGES CXX)

add_executable(library_compare_cpp compare.cpp)
target_compile_features(library_compare_cpp PRIVATE cxx_std_23)
