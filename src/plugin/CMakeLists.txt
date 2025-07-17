set(target CnoidJupyterPlugin)
### only for linux
set(sources
  JupyterPlugin.cpp
  PythonProcess.cpp
  cnoid_interpreter.cpp
  )
set(headers
  JupyterPlugin.h
  PythonProcess.h
  cnoid_interpreter.hpp
  )
if (NOT WIN32)
  add_definitions(-Wno-deprecated-declarations)
endif()
#add_definitions(-DIRSL_DEBUG)

if ( ${CHOREONOID_VERSION_STRING} VERSION_LESS "2.1.0" )
  add_definitions(-DUSE_OLD_OPTION)
endif()

set(xeus_REQUIRED_VERSION 5.0.0)
#set(cppzmq_REQUIRED_VERSION 4.3.0)
find_package(xeus ${xeus_REQUIRED_VERSION} REQUIRED)
#find_package(cppzmq ${cppzmq_REQUIRED_VERSION} REQUIRED)
find_package(Threads)

### using XEUS5
find_package(xeus  REQUIRED)
find_package(xeus-zmq  REQUIRED)
find_package(xeus-python REQUIRED)
find_package(Python3 REQUIRED)

set(xeuslib xeus xeus-zmq xeus-python ${PYTHON_LIBRARY})

add_definitions(-DUSE_XEUS5)
message("using XEUS5 : ${xeus_VERSION} / ${xeuslib}")

## for Qt signal
set(qtheaders PythonProcess.h)
choreonoid_qt_wrap_cpp(qtsources ${qtheaders})
set(sources ${sources} ${qtsources})

if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
  # Build as a master project
  add_definitions(-DEXT_BUNDLE) ## TODO : write node/choreonoid_jupyter.cpp
  set(CMAKE_CXX_STANDARD ${CHOREONOID_CXX_STANDARD})
  choreonoid_add_plugin(${target} ${sources} HEADERS ${headers})
  find_library(LIB_PYTHON_PLUGIN CnoidPythonPlugin HINTS ${CHOREONOID_PLUGIN_DIR})
  Message ("plugin: ${CHOREONOID_PLUGIN_DIR}, lib: ${LIB_PYTHON_PLUGIN}")
  target_link_libraries(${target} Choreonoid::CnoidUtil ${LIB_PYTHON_PLUGIN} ${xeuslib} Threads::Threads)
  ## install
  install(FILES ../../share/jupyter/kernels/choreonoid/kernel.json DESTINATION ${CHOREONOID_SHARE_SUBDIR}/jupyter/kernels/choreonoid)
else()
  # Build as a bundled project
  Message ("Build : JupyterPlugin / under choreonoid/ext")
  ## build
  add_definitions(-DEXT_BUNDLE)
  choreonoid_add_plugin(${target} ${sources} HEADERS ${headers})
  target_link_libraries(${target} PUBLIC CnoidUtil ${xeuslib} Threads::Threads)
  ## install
  install(FILES ../../share/jupyter/kernels/choreonoid/kernel.json DESTINATION ${CHOREONOID_SHARE_SUBDIR}/jupyter/kernels/choreonoid)
endif()
