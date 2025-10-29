# Plotter lib: Plotter

if(EMSCRIPTEN)
    message(STATUS "Emscripten detected, enabling Freetype port for Plotter.")
    add_compile_definitions(USE_FREETYPE_PORT)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_FREETYPE=1")
else()
    find_package(Freetype REQUIRED)
endif()

set(Plotter_src_files
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

add_library(Plotter ${Plotter_src_files})

# Include/link: keep Freetype PRIVATE; DB is PUBLIC if Plotter's headers depend on DB types
if(NOT EMSCRIPTEN)
    target_include_directories(Plotter PRIVATE ${FREETYPE_INCLUDE_DIRS})
    target_link_libraries(Plotter PRIVATE ${FREETYPE_LIBRARIES})
endif()

# Plotter needs DB — expose it PUBLIC if Plotter's public headers use DB types; otherwise PRIVATE
target_link_libraries(Plotter PUBLIC DB)

install(TARGETS Plotter
    EXPORT   shmeaTargets
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# NO install(EXPORT ...) or export(TARGETS ...) here
