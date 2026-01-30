cmake_minimum_required(VERSION 3.21)

add_library(DB STATIC
  lodepng.cpp
  png-helper.cpp
  image.cpp
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

target_include_directories(DB
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/Backend>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Backend>
)

# DB needs core threading/sync objects
target_link_libraries(DB PUBLIC GCore)

# ---- Freetype ----
if(EMSCRIPTEN)
  target_compile_definitions(DB PUBLIC USE_FREETYPE_PORT)
  # Add your emscripten flags as needed, but prefer toolchain handling over CMAKE_CXX_FLAGS
else()
  # Prefer CONFIG (vcpkg) but fall back to module
  find_package(Freetype CONFIG QUIET)
  if(Freetype_FOUND)
    target_link_libraries(DB PUBLIC Freetype::Freetype)
  else()
    find_package(Freetype REQUIRED)
    target_include_directories(DB PUBLIC ${FREETYPE_INCLUDE_DIRS})
    target_link_libraries(DB PUBLIC ${FREETYPE_LIBRARIES})
  endif()
endif()

if(MSVC)
  set_property(TARGET DB PROPERTY MSVC_RUNTIME_LIBRARY "${SHMEA_MSVC_RUNTIME}")
endif()

install(TARGETS DB
  EXPORT   shmeaTargets
  ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
