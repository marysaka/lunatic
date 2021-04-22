cmake_minimum_required(VERSION 3.2)

project(lunatic CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
  placeholder)

set(HEADERS
)

set(HEADERS_PUBLIC
  ../include/lunatic/integer.hpp)

add_library(lunatic STATIC ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})

target_include_directories(lunatic PRIVATE .)
target_include_directories(lunatic PUBLIC ../include)
target_link_libraries(lunatic PRIVATE fmt xbyak)