cmake_minimum_required(VERSION 3.2)
project(lunatic-test CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
  main.cpp)

set(HEADERS
)

add_executable(test ${SOURCES} ${HEADERS})
target_link_libraries(test lunatic)
