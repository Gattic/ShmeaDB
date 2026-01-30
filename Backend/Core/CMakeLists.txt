cmake_minimum_required(VERSION 3.21)

add_library(GCore STATIC
  ../Networking/GMutex.cpp
  ../Networking/GCondition.cpp
  ../Networking/GThread.cpp
)

target_include_directories(GCore
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/Backend>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Backend>
)

find_package(Threads REQUIRED)
target_link_libraries(GCore PUBLIC Threads::Threads)

if(MSVC)
  set_property(TARGET GCore PROPERTY MSVC_RUNTIME_LIBRARY "${SHMEA_MSVC_RUNTIME}")
endif()

install(TARGETS GCore
  EXPORT   shmeaTargets
  ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
