if (DEFINED ENV{TENSORFLOW_DIR})
  add_subdirectory(art)
  add_subdirectory(func)
endif()
add_subdirectory(macro)
