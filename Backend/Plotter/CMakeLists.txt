# Make sure FreeType is available
if(EMSCRIPTEN)
    # Emscripten-specific handling is done in Drawers/CMakeLists.txt
else()
    include(FindPkgConfig)
    find_package(Freetype REQUIRED)
    if(FREETYPE_FOUND)
        include_directories(${FREETYPE_INCLUDE_DIRS})
    else()
        message(FATAL_ERROR "FreeType is required but was not found")
    endif()
endif()

add_subdirectory("Drawers")

set(Plotter_src_files
	PNGPlotter.cpp
)
add_library(Plotter ${Plotter_src_files})

#Link libraries
target_link_libraries(Plotter Drawers)

install(TARGETS Plotter EXPORT shmeaConfig
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})

install(EXPORT shmeaConfig DESTINATION share/shmea/cmake)

export(TARGETS Plotter FILE shmeaConfig.cmake)
