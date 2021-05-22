cmake_minimum_required(VERSION 3.2)

project(lunatic CXX)

# TODO: consider going back to C++14 eventually.
# set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
  backend/x86_64/backend.cpp
  backend/x86_64/register_allocator.cpp
  frontend/ir/emitter.cpp
  frontend/translator/handle/block_data_transfer.cpp
  frontend/translator/handle/branch_exchange.cpp
  frontend/translator/handle/branch_relative.cpp
  frontend/translator/handle/data_processing.cpp
  frontend/translator/handle/halfword_signed_transfer.cpp
  frontend/translator/handle/multiply.cpp
  frontend/translator/handle/multiply_long.cpp
  frontend/translator/handle/single_data_swap.cpp
  frontend/translator/handle/single_data_transfer.cpp
  frontend/translator/handle/status_transfer.cpp
  frontend/translator/translator.cpp
  frontend/basic_block.cpp
  frontend/state.cpp
  jit.cpp)

set(HEADERS
  common/bit.hpp
  common/meta.hpp
  common/optional.hpp
  backend/x86_64/backend.hpp
  backend/x86_64/register_allocator.hpp
  backend/backend.hpp
  frontend/decode/definition/block_data_transfer.hpp
  frontend/decode/definition/branch_relative.hpp
  frontend/decode/definition/branch_exchange.hpp
  frontend/decode/definition/common.hpp
  frontend/decode/definition/data_processing.hpp
  frontend/decode/definition/halfword_signed_transfer.hpp
  frontend/decode/definition/multiply.hpp
  frontend/decode/definition/multiply_long.hpp
  frontend/decode/definition/single_data_swap.hpp
  frontend/decode/definition/single_data_transfer.hpp
  frontend/decode/definition/status_transfer.hpp
  frontend/decode/arm.hpp
  frontend/ir/emitter.hpp
  frontend/ir/opcode.hpp
  frontend/ir/register.hpp
  frontend/ir/value.hpp
  frontend/translator/translator.hpp
  frontend/basic_block.hpp
  frontend/state.hpp)

set(HEADERS_PUBLIC
  ../include/lunatic/detail/meta.hpp
  ../include/lunatic/detail/punning.hpp
  ../include/lunatic/integer.hpp
  ../include/lunatic/jit.hpp
  ../include/lunatic/memory.hpp)

add_library(lunatic STATIC ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})

target_include_directories(lunatic PRIVATE .)
target_include_directories(lunatic PUBLIC ../include)
target_link_libraries(lunatic PRIVATE fmt xbyak)
