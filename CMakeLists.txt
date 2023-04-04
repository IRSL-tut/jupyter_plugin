
if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
  # Build as a master project
  Message ("Build : JupyterPlugin / separated from choreonoid/ext")
  cmake_minimum_required(VERSION 3.10)
  project(JupyterPlugin)
  find_package(Choreonoid REQUIRED)

  #add_subdirectory(src/node)
  add_subdirectory(src/plugin)
else ()
  add_subdirectory(src/plugin)
endif()
