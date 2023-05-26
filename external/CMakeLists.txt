if(NOT LUNATIC_USE_EXTERNAL_FMT)
  add_subdirectory(fmt)
endif()

if (NOT TARGET xbyak)
  add_subdirectory(xbyak)
endif()

if (NOT TARGET oaknut)
  add_subdirectory(oaknut)
endif()
