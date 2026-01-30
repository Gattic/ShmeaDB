cmake_minimum_required(VERSION 3.21)

add_library(GNet STATIC
  crypt.cpp
  connection.cpp
  service.cpp
  socket.cpp
  main.cpp
  # headers don't need to be listed, but it's okay if you do
)

target_include_directories(GNet
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/Backend>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/Backend/Networking>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Backend>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Backend/Networking>
)

find_package(Threads REQUIRED)

# GNet needs core + db; threads via GCore too, but keep explicit for clarity
target_link_libraries(GNet
  PUBLIC
    GCore
    DB
    Threads::Threads
)

# Windows networking must propagate to the final exe
if(WIN32)
  target_link_libraries(GNet PUBLIC ws2_32)
  target_compile_definitions(GNet PUBLIC _WIN32_WINNT=0x0601)
endif()

if(MSVC)
  set_property(TARGET GNet PROPERTY MSVC_RUNTIME_LIBRARY "${SHMEA_MSVC_RUNTIME}")
endif()

install(TARGETS GNet
  EXPORT   shmeaTargets
  ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
