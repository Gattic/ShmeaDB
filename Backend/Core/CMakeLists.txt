set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(Core_src_files
    GMutex.cpp
    GThread.cpp
    GCondVar.cpp
    GDir.cpp
)
add_library(Core ${Core_src_files})

if(WIN32)
    target_link_libraries(Core ws2_32)
endif()

install(TARGETS Core EXPORT shmeaConfig
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})
