cmake_minimum_required(VERSION 2.8)

include_directories ("${PROJECT_SOURCE_DIR}/")

add_library(tinythread tinythread.cpp)

# add the install targets
install (TARGETS tinythread DESTINATION lib)