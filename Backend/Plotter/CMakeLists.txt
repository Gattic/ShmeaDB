cmake_minimum_required(VERSION 3.21)

add_library(Plotter STATIC
  ColorManager.cpp
  SuperSamplingManager.cpp
  ChartLayout.cpp
  ShapeRenderer.cpp
  TextRenderer.cpp
  GridRenderer.cpp
  DataMapper.cpp
  ChartStyler.cpp
  Plotter.cpp
)

target_include_directories(Plotter
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/Backend>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Backend>
)

# Plotter depends on DB; keep PUBLIC if Plotter headers expose DB types
target_link_libraries(Plotter PUBLIC DB)

# ---- Freetype ----
if(EMSCRIPTEN)
  target_compile_definitions(Plotter PUBLIC USE_FREETYPE_PORT)
else()
  find_package(Freetype CONFIG QUIET)
  if(Freetype_FOUND)
    target_link_libraries(Plotter PUBLIC Freetype::Freetype)
  else()
    find_package(Freetype REQUIRED)
    target_include_directories(Plotter PUBLIC ${FREETYPE_INCLUDE_DIRS})
    target_link_libraries(Plotter PUBLIC ${FREETYPE_LIBRARIES})
  endif()
endif()

if(MSVC)
  set_property(TARGET Plotter PROPERTY MSVC_RUNTIME_LIBRARY "${SHMEA_MSVC_RUNTIME}")
endif()

install(TARGETS Plotter
  EXPORT   shmeaTargets
  ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
