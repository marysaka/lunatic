cmake_minimum_required(VERSION 3.2)

project(lunatic CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
  backend/x86_64/backend.cpp
  frontend/ir/emitter.cpp
  frontend/state.cpp
  jit.cpp)

set(HEADERS
  common/bit.hpp
  backend/x86_64/backend.hpp
  backend/backend.hpp
  frontend/ir/emitter.hpp
  frontend/ir/opcode.hpp
  frontend/ir/register.hpp
  frontend/ir/value.hpp
  frontend/state.hpp)

set(HEADERS_PUBLIC
  ../include/lunatic/integer.hpp
  ../include/lunatic/jit.hpp
  ../include/lunatic/memory.hpp)

add_library(lunatic STATIC ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})

target_include_directories(lunatic PRIVATE .)
target_include_directories(lunatic PUBLIC ../include)
target_link_libraries(lunatic PRIVATE fmt xbyak)