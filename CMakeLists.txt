### only for linux
set(sources
  JupyterPlugin.cpp
  PythonProcess.cpp
  JupyterInterpreter.cpp
  )
set(headers
  JupyterPlugin.h
  PythonProcess.h
  JupyterInterpreter.h
  )
add_definitions(-Wno-deprecated-declarations)
add_definitions(-DIRSL_DEBUG)

if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
  # Build as a master project
  Message ("Build : JupyterPlugin / separated from choreonoid/ext")
  cmake_minimum_required(VERSION 3.10)
  project(JupyterPlugin)
  find_package(Choreonoid REQUIRED)
endif()

set(xeus_REQUIRED_VERSION 0.19.1)
#set(cppzmq_REQUIRED_VERSION 4.3.0)
find_package(xeus ${xeus_REQUIRED_VERSION} REQUIRED)
#find_package(cppzmq ${cppzmq_REQUIRED_VERSION} REQUIRED)
find_package(Threads)

## for Qt signal
set(qtheaders PythonProcess.h)
QT5_WRAP_CPP(qtsources ${qtheaders})
set(sources ${sources} ${qtsources})

if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
  # Build as a master project
  set(CMAKE_CXX_STANDARD ${CHOREONOID_CXX_STANDARD})
  choreonoid_add_plugin(CnoidJupyterPlugin ${sources} HEADERS ${headers})
  find_library(LIB_PYTHON_PLUGIN CnoidPythonPlugin HINTS ${CHOREONOID_PLUGIN_DIR})
  Message ("plugin: ${CHOREONOID_PLUGIN_DIR}, lib: ${LIB_PYTHON_PLUGIN}")
  target_link_libraries(CnoidJupyterPlugin Choreonoid::CnoidUtil ${LIB_PYTHON_PLUGIN} xeus Threads::Threads)
  ## install
  install(FILES share/jupyter/kernels/choreonoid/kernel.json DESTINATION ${CHOREONOID_SHARE_SUBDIR}/jupyter/kernels/choreonoid)
else()
  # Build as a bundled project
  Message ("Build : JupyterPlugin / under choreonoid/ext")
  ## build
  choreonoid_add_plugin(CnoidJupyterPlugin ${sources} HEADERS ${headers})
  target_link_libraries(CnoidJupyterPlugin CnoidUtil CnoidPythonPlugin xeus Threads::Threads)
  ## install
  install(FILES share/jupyter/kernels/choreonoid/kernel.json DESTINATION ${CHOREONOID_SHARE_SUBDIR}/jupyter/kernels/choreonoid)
endif()
