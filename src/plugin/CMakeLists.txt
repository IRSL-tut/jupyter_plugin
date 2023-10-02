set(target CnoidJupyterPlugin)
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
if (NOT WIN32)
  add_definitions(-Wno-deprecated-declarations)
endif()
#add_definitions(-DIRSL_DEBUG)

set(xeus_REQUIRED_VERSION 0.19.1)
#set(cppzmq_REQUIRED_VERSION 4.3.0)
find_package(xeus ${xeus_REQUIRED_VERSION} REQUIRED)
#find_package(cppzmq ${cppzmq_REQUIRED_VERSION} REQUIRED)
find_package(Threads)

message("xeus: ${xeus_VERSION}")
if (${xeus_VERSION} VERSION_LESS 3.0.0)
  set(xeuslib xeus)
  message("using XEUS2 : ${xeus_VERSION} / ${xeuslib}")
else()
  find_package(xeus-zmq  REQUIRED)
  set(xeuslib xeus xeus-zmq)
  add_definitions(-DUSE_XEUS3)
  message("using XEUS3 : ${xeus_VERSION} / ${xeuslib}")
endif()

## for Qt signal
set(qtheaders PythonProcess.h)
QT5_WRAP_CPP(qtsources ${qtheaders})
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
  target_link_libraries(${target} PUBLIC CnoidUtil CnoidPythonPlugin ${xeuslib} Threads::Threads)
  ## install
  install(FILES ../../share/jupyter/kernels/choreonoid/kernel.json DESTINATION ${CHOREONOID_SHARE_SUBDIR}/jupyter/kernels/choreonoid)
endif()
