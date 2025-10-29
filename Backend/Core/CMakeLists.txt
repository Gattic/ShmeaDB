# Backend/Core/CMakeLists.txt

cmake_minimum_required(VERSION 3.5.1)

add_library(GCore
  ../Networking/GMutex.cpp
  ../Networking/GCondition.cpp
  ../Networking/GThread.cpp
)

# Public include dirs must be split: build vs install
target_include_directories(GCore
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/Backend>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/Backend/Networking>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Backend>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Backend/Networking>
)

find_package(Threads REQUIRED)
target_link_libraries(GCore PUBLIC ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS GCore
  EXPORT   shmeaTargets
  RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
