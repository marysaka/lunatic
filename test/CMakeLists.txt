cmake_minimum_required(VERSION 3.2)
project(lunatic-test CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
  arm/tablegen/tablegen.cpp
  arm/arm.cpp
  main.cpp)

set(HEADERS
  arm/handlers/arithmetic.inl
  arm/handlers/handler16.inl
  arm/handlers/handler32.inl
  arm/handlers/memory.inl
  arm/tablegen/decoder.hpp
  arm/tablegen/gen_arm.hpp
  arm/tablegen/gen_thumb.hpp
  arm/util/likely.hpp
  arm/util/meta.hpp
  arm/util/punning.hpp)

add_executable(test ${SOURCES} ${HEADERS})
target_link_libraries(test lunatic)
