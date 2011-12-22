cmake_minimum_required(VERSION 2.8)

include_directories ("${PROJECT_SOURCE_DIR}/")

add_library(linenoise linenoise.cpp utf8.cpp)

# add the install targets
install (TARGETS linenoise DESTINATION lib)