cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0003 NEW)
cmake_policy(SET CMP0057 NEW)

project(CnoidJupyter C CXX)

set(target CnoidJupyter)

### only for linux
set(sources
  PythonProcess.cpp
  cnoid_interpreter.cpp
  )
set(headers
  PythonProcess.h
  cnoid_interpreter.hpp
  )
if (NOT WIN32)
  add_definitions(-Wno-deprecated-declarations)
endif()
#add_definitions(-DIRSL_DEBUG)

#set(cppzmq_REQUIRED_VERSION 4.3.0)
#find_package(cppzmq ${cppzmq_REQUIRED_VERSION} REQUIRED)

### using XEUS5
set(xeus_REQUIRED_VERSION 5.0.0)
find_package(xeus ${xeus_REQUIRED_VERSION} REQUIRED)
find_package(xeus-zmq  REQUIRED)
find_package(xeus-python REQUIRED)
find_package(Python3 REQUIRED)

#Qt
#find_package(Threads)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

set(xeuslib xeus xeus-zmq xeus-python ${PYTHON_LIBRARY})

add_definitions(-DUSE_XEUS5)
message("using XEUS5 : ${xeus_VERSION} / ${xeuslib}")

## for Qt signal
set(qtheaders PythonProcess.h)
#choreonoid_qt_wrap_cpp(qtsources ${qtheaders})
qt6_wrap_cpp(qtsources ${qtheaders})

set(sources ${sources} ${qtsources})

if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
  # Build as a master project
  add_library(${target} SHARED ${sources})
  target_link_libraries( ${target} ${xeuslib} Qt6::Core Qt6::Gui)

  add_executable(test testmain.cpp)
  target_link_libraries( test Qt6::Core Qt6::Gui Qt6::Widgets ${target} )
  ## install
  ## install(FILES ../../share/jupyter/kernels/choreonoid/kernel.json DESTINATION ${CHOREONOID_SHARE_SUBDIR}/jupyter/kernels/choreonoid)
else()
  # Build as a bundled project
  Message ("Build : JupyterPlugin / under choreonoid/ext")
  ## build
  choreonoid_add_plugin(${target} ${sources} HEADERS ${headers})
  target_link_libraries(${target} PUBLIC CnoidUtil ${xeuslib} Threads::Threads)
  ## install
  install(FILES ../../share/jupyter/kernels/choreonoid/kernel.json DESTINATION ${CHOREONOID_SHARE_SUBDIR}/jupyter/kernels/choreonoid)
endif()
