cmake_minimum_required(VERSION 3.22.1)
project(cheat CXX)
add_library(cheat SHARED injector.cpp)
target_link_libraries(cheat log android dl)
