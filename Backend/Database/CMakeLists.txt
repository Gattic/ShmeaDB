# Database lib: DB

if(EMSCRIPTEN)
  message(STATUS "Emscripten detected, enabling Freetype port for DB.")
  add_compile_definitions(USE_FREETYPE_PORT)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_FREETYPE=1")
else()
  find_package(Freetype REQUIRED)
endif()

add_library(DB
  lodepng.cpp
  png-helper.cpp
  image.cpp
  GVector.h
  GPointer.h
  GType.cpp
  GType_helpers.cpp
  GType_operators.cpp
  GString.cpp
  GString_helpers.cpp
  GList.cpp
  GLogger.cpp
  GTable.cpp
  GObject.cpp
  maxid.cpp
  SaveTable.cpp
  SaveFolder.cpp
  Serializable.cpp
  ServiceData.cpp
  standardizable.cpp
  PNGPlotter.cpp
)

# Freetype stays PRIVATE
if(NOT EMSCRIPTEN)
  target_include_directories(DB PRIVATE ${FREETYPE_INCLUDE_DIRS})
  target_link_libraries(DB PRIVATE ${FREETYPE_LIBRARIES})
endif()

# NEW: DB needs the core threading symbols (GMutex etc.)
target_link_libraries(DB PRIVATE GCore)

install(TARGETS DB
  EXPORT   shmeaTargets
  RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
