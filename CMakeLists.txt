cmake_minimum_required(VERSION 3.2)

# Determine whether this is a standalone project or included by other projects
set(IS_SUBPROJECT ON)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(IS_SUBPROJECT OFF)
endif()

option(LUNATIC_USE_EXTERNAL_FMT "Use externally provided {fmt} library." OFF)

project(lunatic-root)

add_subdirectory(external)
add_subdirectory(src)
if(NOT IS_SUBPROJECT)
  add_subdirectory(test)
endif()
