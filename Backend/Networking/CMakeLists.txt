# Networking lib: GNet

find_package(Threads REQUIRED)

set(GNet_src_files
  platform.h
  GThread.h        # headers still included by users of GNet
  GCondition.h
  GMutex.h
  main.cpp
  main.h
  crypt.cpp
  crypt.h
  connection.cpp
  connection.h
  service.cpp
  service.h
  socket.cpp
  socket.h
  # IMPORTANT: DO NOT compile these here anymore:
  # GThread.cpp
  # GCondition.cpp
  # GMutex.cpp
)

add_library(GNet ${GNet_src_files})

# Link: core first, then DB (if public headers expose DB types, use PUBLIC; otherwise PRIVATE)
target_link_libraries(GNet
  PRIVATE
    GCore
    DB
    ${CMAKE_THREAD_LIBS_INIT}
)

if(WIN32)
  target_link_libraries(GNet PRIVATE ws2_32)
endif()

install(TARGETS GNet
  EXPORT   shmeaTargets
  RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
