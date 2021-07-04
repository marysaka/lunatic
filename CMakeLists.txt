cmake_minimum_required(VERSION 3.2)

# Determine whether this is a standalone project or included by other projects
if(DEFINED PROJECT_NAME)
  set(IS_SUBPROJECT ON)
else()
  set(IS_SUBPROJECT OFF)
endif()

project(lunatic-root)

add_subdirectory(external)
add_subdirectory(src)
if(NOT IS_SUBPROJECT)
  add_subdirectory(test)
endif()
